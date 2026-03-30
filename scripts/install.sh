#!/bin/bash
# =============================================================================
# my_install.sh — AppDir 模式安装脚本
#
# 设计思路:
#   1. 原子化目录: 所有程序文件收纳在 INSTALL_ROOT (默认 /opt/fast_cpp_server) 下。
#      卸载时只需 rm -rf 该目录 + service 文件，系统干干净净。
#   2. 软链接替代硬拷贝: /usr/local/bin 下只放一个指向 INSTALL_ROOT/bin 的软链接，
#      mosquitto 等系统组件也优先软链接，避免"版本孤岛"。
#   3. LD_LIBRARY_PATH 注入: 在生成的 systemd service 中设置 Environment，
#      私有库不污染系统 /etc/ld.so.conf。
#   4. 配置保护: 升级时不覆盖已有 config.ini，只在首次安装时写入。
#   5. 日志规范化: 日志放 /var/log/${PROGRAM_NAME}，systemd 用 append 模式。
#
# 用法:
#   sudo ./my_install.sh                          # 默认安装到 /opt/fast_cpp_server
#   sudo ./my_install.sh --install-root /opt/myapp # 指定安装根目录
#   sudo ./my_install.sh --debug                   # 仅打印命令，不实际执行
#   sudo ./my_install.sh --debug --install-root /tmp/test
# =============================================================================
set -euo pipefail

# ========================= 常量定义 =========================
A_PROGRAM_NAME="Fast Cpp Server"
PROGRAM_NAME="fast_cpp_server"
MQTT_PROGRAM_NAME="mosquitto"
MEDIAMTX_PROGRAM_NAME="mediamtx"
SERVICE_NAME="${PROGRAM_NAME}.service"

# ========================= 默认值 =========================
DEFAULT_INSTALL_ROOT="/opt/${PROGRAM_NAME}"
INSTALL_ROOT="${DEFAULT_INSTALL_ROOT}"
DEBUG=false
SUPER="sudo"

# ========================= 颜色常量 =========================
C_RESET="\033[0m"
C_GREEN="\033[32m"
C_YELLOW="\033[33m"
C_RED="\033[31m"
C_MAGENTA="\033[35m"
C_CYAN="\033[36m"
C_BLUE="\033[34m"

# ========================= 日志函数 =========================

# tag: 给日志打上彩色标签
tag() {
  case "${1:-}" in
    dev)  echo -e "${C_MAGENTA}[dev]${C_RESET}" ;;
    pro)  echo -e "${C_GREEN}[pro]${C_RESET}" ;;
    info) echo -e "${C_CYAN}[info]${C_RESET}" ;;
    warn) echo -e "${C_YELLOW}[warn]${C_RESET}" ;;
    err)  echo -e "${C_RED}[err]${C_RESET}" ;;
    ok)   echo -e "${C_GREEN}[ ok ]${C_RESET}" ;;
    *)    echo -e "[log]" ;;
  esac
}

# log_line: 同时输出到终端和日志文件
log_line() {
  local level="$1"; shift
  local msg="$*"
  # 确保日志目录存在 (初始化阶段可能还没创建)
  mkdir -p "$(dirname "${LOG_FILE}")" >/dev/null 2>&1 || true
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] [${level}] ${msg}" >> "${LOG_FILE}" 2>/dev/null || true
  echo -e "$(tag "${level}") ${msg}"
}

# run_cmd: 执行命令并记录; debug 模式下只打印不执行
run_cmd() {
  local cmd="$*"
  if $DEBUG; then
    log_line dev "[DRY-RUN] ${cmd}"
    return 0
  else
    log_line pro "${cmd}"
    set +e
    bash -c "${cmd}" >> "${LOG_FILE}" 2>&1
    local rc=$?
    set -e
    if [ $rc -ne 0 ]; then
      log_line err "Command failed (rc=${rc}): ${cmd}"
      log_line err "See log for details: ${LOG_FILE}"
      exit $rc
    fi
    return 0
  fi
}

# ========================= 参数解析 =========================
# 支持:
#   --debug             dry-run 模式
#   --install-root DIR  指定安装根目录
parse_args() {
  while [ $# -gt 0 ]; do
    case "$1" in
      --debug)
        DEBUG=true
        shift
        ;;
      --install-root)
        if [ -z "${2:-}" ]; then
          echo -e "${C_RED}[err]${C_RESET} --install-root requires a directory argument"
          exit 1
        fi
        INSTALL_ROOT="$2"
        shift 2
        ;;
      --help|-h)
        echo "Usage: $0 [--debug] [--install-root DIR]"
        echo ""
        echo "Options:"
        echo "  --install-root DIR  Install root directory (default: ${DEFAULT_INSTALL_ROOT})"
        echo "  --debug             Dry-run mode, only print commands"
        echo "  -h, --help          Show this help message"
        exit 0
        ;;
      *)
        echo -e "${C_RED}[err]${C_RESET} Unknown option: $1"
        exit 1
        ;;
    esac
  done
}

# ========================= 前置检查 =========================
need_sudo_or_die() {
  if $DEBUG; then return 0; fi
  if [ "$(id -u)" -eq 0 ]; then
    # 已经是 root，不需要 sudo
    SUPER=""
    return 0
  fi
  if ! sudo -n true 2>/dev/null; then
    log_line err "需要 root 权限。请使用 sudo 运行此脚本。"
    exit 1
  fi
}

# ========================= 路径定义 (依赖 INSTALL_ROOT) =========================
setup_paths() {
  # --- AppDir 内部路径 ---
  BIN_DIR="${INSTALL_ROOT}/bin"          # 可执行文件
  LIB_DIR="${INSTALL_ROOT}/lib"          # 私有动态库
  CONFIG_DIR="${INSTALL_ROOT}/config"    # 配置文件
  SHARE_DIR="${INSTALL_ROOT}/share"      # swagger-res 等资源
  SERVICE_DIR="${INSTALL_ROOT}/service"  # service 文件归档副本
  ETC_DIR="${INSTALL_ROOT}/etc"          # mosquitto etc 等

  # --- 系统路径 (只放软链接和日志) ---
  SYMLINK_PATH="/usr/local/bin/${PROGRAM_NAME}"          # 主程序软链接
  SYSTEMD_SERVICE_PATH="/etc/systemd/system/${SERVICE_NAME}"
  LOG_DIR="/var/log/${PROGRAM_NAME}"                     # 标准化日志目录

  # --- 安装脚本日志 ---
  TS="$(date '+%Y%m%d_%H%M%S')"
  LOG_FILE="${LOG_DIR}/install_${TS}.log"
}

# ========================= 打印安装计划 =========================
print_plan() {
  log_line info "=============================================="
  log_line info "  AppDir Install Plan"
  log_line info "=============================================="
  log_line info "PROGRAM_NAME:     ${PROGRAM_NAME}"
  log_line info "DEBUG:            ${DEBUG}"
  log_line info "INSTALL_ROOT:     ${INSTALL_ROOT}"
  log_line info "----------------------------------------------"
  log_line info "BIN_DIR:          ${BIN_DIR}"
  log_line info "LIB_DIR:          ${LIB_DIR}"
  log_line info "CONFIG_DIR:       ${CONFIG_DIR}"
  log_line info "SHARE_DIR:        ${SHARE_DIR}"
  log_line info "SERVICE_DIR:      ${SERVICE_DIR}"
  log_line info "ETC_DIR:          ${ETC_DIR}"
  log_line info "----------------------------------------------"
  log_line info "SYMLINK_PATH:     ${SYMLINK_PATH}"
  log_line info "SYSTEMD SERVICE:  ${SYSTEMD_SERVICE_PATH}"
  log_line info "LOG_DIR:          ${LOG_DIR}"
  log_line info "=============================================="
}

# ========================= 定位 release 包中的文件 =========================
# 脚本应从解压后的 release 包根目录运行 (cd 进去后执行)
locate_release_dir() {
  RELEASE_DIR="$(pwd)"

  # 检查关键文件是否存在
  if [ ! -f "${RELEASE_DIR}/bin/${PROGRAM_NAME}" ]; then
    log_line err "在 ${RELEASE_DIR}/bin/ 下未找到 ${PROGRAM_NAME} 可执行文件"
    log_line err "请 cd 到解压后的 release 包根目录，然后运行此脚本"
    log_line err "例: cd releases/fast_cpp_server-xxx && /path/to/my_install.sh"
    exit 1
  fi
  log_line info "Release directory: ${RELEASE_DIR}"
}

# ========================= 安装步骤 =========================

# Step 1: 停止已有服务
stop_existing_service() {
  log_line info "[Step 1] 停止已有服务 (如果存在)..."
  run_cmd "${SUPER} systemctl stop ${SERVICE_NAME} 2>/dev/null || true"
  run_cmd "${SUPER} systemctl disable ${SERVICE_NAME} 2>/dev/null || true"
  log_line ok "已有服务已停止"
}

# Step 2: 创建 AppDir 目录结构
create_directories() {
  log_line info "[Step 2] 创建 AppDir 目录结构..."

  local dirs=(
    "${INSTALL_ROOT}"
    "${BIN_DIR}"
    "${LIB_DIR}"
    "${CONFIG_DIR}"
    "${SHARE_DIR}"
    "${SERVICE_DIR}"
    "${ETC_DIR}"
    "${LOG_DIR}"
  )
  for d in "${dirs[@]}"; do
    run_cmd "${SUPER} mkdir -p ${d}"
    log_line info "  -> 创建目录: ${d}"
  done

  # 日志目录需要可写权限
  run_cmd "${SUPER} chmod 755 ${LOG_DIR}"
  log_line ok "目录结构创建完成"
}

# Step 3: 安装主程序和附属二进制
install_binaries() {
  log_line info "[Step 3] 安装可执行文件到 ${BIN_DIR}/ ..."

  # 3a. 主程序 (直接拷贝)
  log_line info "  -> 安装主程序: ${PROGRAM_NAME}"
  run_cmd "${SUPER} cp -f ${RELEASE_DIR}/bin/${PROGRAM_NAME} ${BIN_DIR}/${PROGRAM_NAME}"
  run_cmd "${SUPER} chmod 755 ${BIN_DIR}/${PROGRAM_NAME}"

  # 3b. mediamtx (直接拷贝，来自 release 包)
  if [ -f "${RELEASE_DIR}/bin/${MEDIAMTX_PROGRAM_NAME}" ]; then
    log_line info "  -> 安装 mediamtx (从 release 包)"
    run_cmd "${SUPER} cp -f ${RELEASE_DIR}/bin/${MEDIAMTX_PROGRAM_NAME} ${BIN_DIR}/${MEDIAMTX_PROGRAM_NAME}"
    run_cmd "${SUPER} chmod 755 ${BIN_DIR}/${MEDIAMTX_PROGRAM_NAME}"
  else
    log_line warn "  -> 未在 release 包中找到 mediamtx，跳过"
  fi

  # 3c. mosquitto — 优先软链接系统版本，避免"版本孤岛"
  install_mosquitto_binary

  log_line ok "可执行文件安装完成"
}

# 安装 mosquitto: 优先软链接系统已有的版本
install_mosquitto_binary() {
  local system_mosquitto=""

  # 检查系统是否已有 mosquitto
  if command -v "${MQTT_PROGRAM_NAME}" >/dev/null 2>&1; then
    system_mosquitto="$(command -v "${MQTT_PROGRAM_NAME}")"
  fi

  if [ -n "${system_mosquitto}" ]; then
    # 系统中已有 mosquitto，使用软链接 — 跟随系统升级
    log_line info "  -> 发现系统 mosquitto: ${system_mosquitto}，创建软链接"
    run_cmd "${SUPER} ln -sf ${system_mosquitto} ${BIN_DIR}/${MQTT_PROGRAM_NAME}"
  elif [ -f "${RELEASE_DIR}/bin/${MQTT_PROGRAM_NAME}" ]; then
    # 系统中没有，使用 release 包中的版本
    log_line info "  -> 系统中未找到 mosquitto，从 release 包安装"
    run_cmd "${SUPER} cp -f ${RELEASE_DIR}/bin/${MQTT_PROGRAM_NAME} ${BIN_DIR}/${MQTT_PROGRAM_NAME}"
    run_cmd "${SUPER} chmod 755 ${BIN_DIR}/${MQTT_PROGRAM_NAME}"
  elif [ -f "${RELEASE_DIR}/sbin/${MQTT_PROGRAM_NAME}" ]; then
    # 有些 release 包把 mosquitto 放在 sbin
    log_line info "  -> 从 release 包 sbin/ 安装 mosquitto"
    run_cmd "${SUPER} cp -f ${RELEASE_DIR}/sbin/${MQTT_PROGRAM_NAME} ${BIN_DIR}/${MQTT_PROGRAM_NAME}"
    run_cmd "${SUPER} chmod 755 ${BIN_DIR}/${MQTT_PROGRAM_NAME}"
  else
    log_line warn "  -> 未找到 mosquitto 可执行文件，跳过"
  fi
}

# Step 4: 安装私有库文件
install_libraries() {
  log_line info "[Step 4] 安装库文件到 ${LIB_DIR}/ ..."

  if [ -d "${RELEASE_DIR}/lib" ]; then
    # 只拷贝 .so 文件和符号链接，跳过 .a / .pc / pkgconfig
    run_cmd "${SUPER} find ${RELEASE_DIR}/lib -maxdepth 1 \\( -name '*.so' -o -name '*.so.*' \\) -exec cp -a {} ${LIB_DIR}/ \\;"
    log_line info "  -> 已拷贝 .so 库文件 (保留符号链接)"
  else
    log_line warn "  -> release 包中无 lib/ 目录，跳过"
  fi

  log_line ok "库文件安装完成"
}

# Step 5: 安装配置文件 (带保护机制)
install_configs() {
  log_line info "[Step 5] 安装配置文件到 ${CONFIG_DIR}/ ..."

  if [ ! -d "${RELEASE_DIR}/config" ]; then
    log_line warn "  -> release 包中无 config/ 目录，跳过"
    return
  fi

  # 遍历 release 包中的配置文件
  for src_file in "${RELEASE_DIR}"/config/*; do
    [ -f "${src_file}" ] || continue
    local filename
    filename="$(basename "${src_file}")"
    local dest_file="${CONFIG_DIR}/${filename}"

    # 配置保护: 如果目标文件已存在，不覆盖 (升级时保护用户配置)
    if [ -f "${dest_file}" ]; then
      log_line warn "  -> 配置文件已存在，跳过覆盖: ${dest_file}"
      # 保存新版本为 .new 供用户对比
      run_cmd "${SUPER} cp -f ${src_file} ${dest_file}.new"
      log_line info "     新版本已保存为: ${dest_file}.new (供手动对比)"
    else
      log_line info "  -> 安装配置文件: ${filename}"
      run_cmd "${SUPER} cp -f ${src_file} ${dest_file}"
    fi
    run_cmd "${SUPER} chmod 644 ${dest_file} 2>/dev/null || true"
  done

  # 替换 config.ini 中的占位符为实际安装路径
  patch_config_ini

  log_line ok "配置文件安装完成"
}

# 替换 config.ini 中的占位符 (__CONFIG_DIR__ 等) 为实际的 AppDir 路径
# 无论是首次安装还是升级 (.new)，都需要做路径替换
patch_config_ini() {
  local config_file="${CONFIG_DIR}/config.ini"
  local config_new="${CONFIG_DIR}/config.ini.new"

  # 定义占位符 -> 实际路径的映射
  #   __CONFIG_DIR__      -> /opt/fast_cpp_server/config
  #   __SWAGGER_RES_DIR__ -> /opt/fast_cpp_server/share/swagger-res/res
  #   __LOGGER_DIR__      -> /var/log/fast_cpp_server/
  local real_config_dir="${CONFIG_DIR}"
  local real_swagger_dir="${SHARE_DIR}/swagger-res/res"
  local real_logger_dir="${LOG_DIR}/"

  log_line info "  -> 替换 config.ini 中的路径占位符..."
  log_line info "     __CONFIG_DIR__      => ${real_config_dir}"
  log_line info "     __SWAGGER_RES_DIR__ => ${real_swagger_dir}"
  log_line info "     __LOGGER_DIR__      => ${real_logger_dir}"

  # 对主配置文件做替换 (首次安装时该文件刚拷贝过去，含占位符)
  if [ -f "${config_file}" ]; then
    # 使用 sed 替换占位符; 分隔符用 | 避免路径中的 / 冲突
    run_cmd "${SUPER} sed -i \
      -e 's|__CONFIG_DIR__|${real_config_dir}|g' \
      -e 's|__SWAGGER_RES_DIR__|${real_swagger_dir}|g' \
      -e 's|__LOGGER_DIR__|${real_logger_dir}|g' \
      ${config_file}"
    log_line info "  -> 已替换: ${config_file}"
  fi

  # 如果升级场景产生了 .new 文件，也做替换方便用户对比
  if [ -f "${config_new}" ]; then
    run_cmd "${SUPER} sed -i \
      -e 's|__CONFIG_DIR__|${real_config_dir}|g' \
      -e 's|__SWAGGER_RES_DIR__|${real_swagger_dir}|g' \
      -e 's|__LOGGER_DIR__|${real_logger_dir}|g' \
      ${config_new}"
    log_line info "  -> 已替换: ${config_new}"
  fi
}

# Step 6: 安装资源文件 (swagger-res 等)
install_resources() {
  log_line info "[Step 6] 安装资源文件到 ${SHARE_DIR}/ ..."

  # swagger-res
  if [ -d "${RELEASE_DIR}/swagger-res" ]; then
    run_cmd "${SUPER} cp -rf ${RELEASE_DIR}/swagger-res ${SHARE_DIR}/"
    log_line info "  -> swagger-res 已安装"
  else
    log_line warn "  -> 未找到 swagger-res，跳过"
  fi

  # mosquitto etc 配置
  if [ -d "${RELEASE_DIR}/etc" ]; then
    run_cmd "${SUPER} cp -rf ${RELEASE_DIR}/etc/* ${ETC_DIR}/"
    log_line info "  -> etc/ (mosquitto 配置) 已安装"
  else
    log_line info "  -> 无 etc/ 目录，跳过"
  fi

  log_line ok "资源文件安装完成"
}

# Step 7: 创建 /usr/local/bin 软链接
create_symlinks() {
  log_line info "[Step 7] 创建软链接..."

  # 主程序软链接
  run_cmd "${SUPER} ln -sf ${BIN_DIR}/${PROGRAM_NAME} ${SYMLINK_PATH}"
  log_line info "  -> ${SYMLINK_PATH} -> ${BIN_DIR}/${PROGRAM_NAME}"

  log_line ok "软链接创建完成"
}

# Step 8: 生成并安装 systemd service 文件
install_systemd_service() {
  log_line info "[Step 8] 生成并安装 systemd service..."

  # 动态生成 service 文件，路径全部指向 INSTALL_ROOT
  local service_content
  read -r -d '' service_content <<SERVICEEOF || true
[Unit]
Description=${A_PROGRAM_NAME} Service (AppDir)
After=network.target

[Service]
# 以 root 运行
User=root
Group=root
Type=simple

# 设置 LD_LIBRARY_PATH 注入私有库，不污染系统 ld.so.conf
Environment="LD_LIBRARY_PATH=${LIB_DIR}"

# 工作目录设为 AppDir 根
WorkingDirectory=${INSTALL_ROOT}

# 启动前确保日志目录存在
ExecStartPre=/usr/bin/mkdir -p ${LOG_DIR}

# 启动主程序
ExecStart=${BIN_DIR}/${PROGRAM_NAME} -c ${CONFIG_DIR}/config.ini

# 停止行为: 发送 SIGTERM，等待 20s 后强杀
KillMode=control-group
KillSignal=SIGTERM
TimeoutStopSec=20s

# 失败时自动重启
Restart=on-failure
RestartSec=20s

# 日志输出到文件 (append 模式)，比脚本手动 >> 更可靠
StandardOutput=append:${LOG_DIR}/${PROGRAM_NAME}.log
StandardError=append:${LOG_DIR}/${PROGRAM_NAME}.err.log

# 运行时目录
RuntimeDirectory=${PROGRAM_NAME}

# 允许写入日志目录
ReadWritePaths=${LOG_DIR}

# 资源限制
LimitNOFILE=65535
LimitCORE=infinity

[Install]
WantedBy=multi-user.target
SERVICEEOF

  # 写入到 AppDir 内归档一份
  if $DEBUG; then
    log_line dev "[DRY-RUN] 生成 service 文件到 ${SERVICE_DIR}/${SERVICE_NAME}"
  else
    echo "${service_content}" | ${SUPER} tee "${SERVICE_DIR}/${SERVICE_NAME}" > /dev/null
    log_line info "  -> 归档 service 文件: ${SERVICE_DIR}/${SERVICE_NAME}"
  fi

  # 安装到 systemd 目录
  if $DEBUG; then
    log_line dev "[DRY-RUN] 安装 service 到 ${SYSTEMD_SERVICE_PATH}"
  else
    echo "${service_content}" | ${SUPER} tee "${SYSTEMD_SERVICE_PATH}" > /dev/null
    ${SUPER} chmod 644 "${SYSTEMD_SERVICE_PATH}"
    log_line info "  -> 已安装: ${SYSTEMD_SERVICE_PATH}"
  fi

  log_line ok "systemd service 安装完成"
}

# Step 9: 启用并启动服务
enable_and_start_service() {
  log_line info "[Step 9] 启用并启动服务..."

  run_cmd "${SUPER} systemctl daemon-reload"
  log_line info "  -> systemctl daemon-reload 完成"

  run_cmd "${SUPER} systemctl enable ${SERVICE_NAME}"
  log_line info "  -> 服务已启用 (开机自启)"

  run_cmd "${SUPER} systemctl start ${SERVICE_NAME}"
  log_line info "  -> 服务已启动"

  # 检查启动状态
  if ! $DEBUG; then
    set +e
    ${SUPER} systemctl status "${SERVICE_NAME}" --no-pager >> "${LOG_FILE}" 2>&1
    local rc=$?
    set -e
    if [ $rc -eq 0 ]; then
      log_line ok "服务运行正常"
    else
      log_line warn "服务状态异常 (rc=${rc})，请检查: journalctl -u ${SERVICE_NAME}"
    fi
  fi

  log_line ok "服务启动流程完成"
}

# ========================= 安装完成摘要 =========================
print_summary() {
  log_line info "=============================================="
  log_line info "  安装完成 — 摘要"
  log_line info "=============================================="
  log_line info "安装根目录:       ${INSTALL_ROOT}"
  log_line info "主程序:           ${BIN_DIR}/${PROGRAM_NAME}"
  log_line info "软链接:           ${SYMLINK_PATH} -> ${BIN_DIR}/${PROGRAM_NAME}"
  log_line info "库文件:           ${LIB_DIR}/"
  log_line info "配置文件:         ${CONFIG_DIR}/"
  log_line info "日志目录:         ${LOG_DIR}/"
  log_line info "systemd service:  ${SYSTEMD_SERVICE_PATH}"
  log_line info "----------------------------------------------"
  log_line info "管理命令:"
  log_line info "  查看状态:  sudo systemctl status ${SERVICE_NAME}"
  log_line info "  查看日志:  sudo journalctl -u ${SERVICE_NAME} -f"
  log_line info "  停止服务:  sudo systemctl stop ${SERVICE_NAME}"
  log_line info "  卸载:      sudo ./my_uninstall.sh --install-root ${INSTALL_ROOT}"
  log_line info "=============================================="
}

# ========================= 主流程 =========================
main() {
  parse_args "$@"
  setup_paths

  # 确保日志目录尽早创建 (后续 log_line 需要)
  mkdir -p "$(dirname "${LOG_FILE}")" 2>/dev/null || true

  log_line info "========== my_install.sh 开始执行 =========="
  locate_release_dir
  print_plan
  need_sudo_or_die

  stop_existing_service      # Step 1
  create_directories         # Step 2
  install_binaries           # Step 3
  install_libraries          # Step 4
  install_configs            # Step 5
  install_resources          # Step 6
  create_symlinks            # Step 7
  install_systemd_service    # Step 8
  enable_and_start_service   # Step 9

  print_summary
  log_line info "========== my_install.sh 执行完毕 =========="
}

main "$@"
