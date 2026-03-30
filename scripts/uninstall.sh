#!/bin/bash
# =============================================================================
# my_uninstall.sh — AppDir 模式卸载脚本
#
# 与 my_install.sh 配套使用。
# 清理内容:
#   1. 停止并禁用 systemd 服务
#   2. 删除 systemd service 文件
#   3. 删除 /usr/local/bin 下的软链接
#   4. 删除整个 INSTALL_ROOT 目录 (原子化卸载)
#   5. 可选: 清理日志目录
#
# 用法:
#   sudo ./my_uninstall.sh                            # 默认卸载 /opt/fast_cpp_server
#   sudo ./my_uninstall.sh --install-root /opt/myapp  # 指定安装根目录
#   sudo ./my_uninstall.sh --keep-logs                # 保留日志目录
#   sudo ./my_uninstall.sh --keep-config              # 保留配置文件
#   sudo ./my_uninstall.sh --debug                    # 仅打印，不实际执行
# =============================================================================
set -euo pipefail

# ========================= 常量定义 =========================
PROGRAM_NAME="fast_cpp_server"
SERVICE_NAME="${PROGRAM_NAME}.service"

# ========================= 默认值 =========================
DEFAULT_INSTALL_ROOT="/opt/${PROGRAM_NAME}"
INSTALL_ROOT="${DEFAULT_INSTALL_ROOT}"
DEBUG=false
KEEP_LOGS=false
KEEP_CONFIG=false
SUPER="sudo"

# ========================= 颜色常量 =========================
C_RESET="\033[0m"
C_GREEN="\033[32m"
C_YELLOW="\033[33m"
C_RED="\033[31m"
C_MAGENTA="\033[35m"
C_CYAN="\033[36m"

# ========================= 日志函数 =========================

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

log_line() {
  local level="$1"; shift
  local msg="$*"
  # 卸载时日志写到临时文件，因为日志目录可能即将被删除
  if [ -n "${LOG_FILE:-}" ]; then
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [${level}] ${msg}" >> "${LOG_FILE}" 2>/dev/null || true
  fi
  echo -e "$(tag "${level}") ${msg}"
}

run_cmd() {
  local cmd="$*"
  if $DEBUG; then
    log_line dev "[DRY-RUN] ${cmd}"
    return 0
  else
    log_line pro "${cmd}"
    set +e
    bash -c "${cmd}" >> "${LOG_FILE:-/dev/null}" 2>&1
    local rc=$?
    set -e
    if [ $rc -ne 0 ]; then
      log_line warn "Command returned non-zero (rc=${rc}): ${cmd} (继续卸载)"
    fi
    return 0
  fi
}

# ========================= 参数解析 =========================
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
      --keep-logs)
        KEEP_LOGS=true
        shift
        ;;
      --keep-config)
        KEEP_CONFIG=true
        shift
        ;;
      --help|-h)
        echo "Usage: $0 [--debug] [--install-root DIR] [--keep-logs] [--keep-config]"
        echo ""
        echo "Options:"
        echo "  --install-root DIR  Install root directory (default: ${DEFAULT_INSTALL_ROOT})"
        echo "  --keep-logs         Keep log directory /var/log/${PROGRAM_NAME}"
        echo "  --keep-config       Keep config directory ${DEFAULT_INSTALL_ROOT}/config (backup)"
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

# ========================= 路径定义 =========================
setup_paths() {
  BIN_DIR="${INSTALL_ROOT}/bin"
  LIB_DIR="${INSTALL_ROOT}/lib"
  CONFIG_DIR="${INSTALL_ROOT}/config"

  SYMLINK_PATH="/usr/local/bin/${PROGRAM_NAME}"
  SYSTEMD_SERVICE_PATH="/etc/systemd/system/${SERVICE_NAME}"
  LOG_DIR="/var/log/${PROGRAM_NAME}"

  # 卸载日志写到 /tmp，因为 LOG_DIR 可能会被删除
  LOG_FILE="/tmp/${PROGRAM_NAME}_uninstall_$(date '+%Y%m%d_%H%M%S').log"
}

# ========================= 前置检查 =========================
need_sudo_or_die() {
  if $DEBUG; then return 0; fi
  if [ "$(id -u)" -eq 0 ]; then
    SUPER=""
    return 0
  fi
  if ! sudo -n true 2>/dev/null; then
    log_line err "需要 root 权限。请使用 sudo 运行此脚本。"
    exit 1
  fi
}

# ========================= 卸载步骤 =========================

# Step 1: 停止并禁用服务
stop_service() {
  log_line info "[Step 1] 停止并禁用 systemd 服务..."

  if systemctl is-active "${SERVICE_NAME}" >/dev/null 2>&1; then
    run_cmd "${SUPER} systemctl stop ${SERVICE_NAME}"
    log_line info "  -> 服务已停止"
  else
    log_line info "  -> 服务未运行，跳过"
  fi

  if systemctl is-enabled "${SERVICE_NAME}" >/dev/null 2>&1; then
    run_cmd "${SUPER} systemctl disable ${SERVICE_NAME}"
    log_line info "  -> 服务已禁用"
  else
    log_line info "  -> 服务未启用，跳过"
  fi

  log_line ok "服务已停止并禁用"
}

# Step 2: 删除 systemd service 文件
remove_service_file() {
  log_line info "[Step 2] 删除 systemd service 文件..."

  if [ -f "${SYSTEMD_SERVICE_PATH}" ]; then
    run_cmd "${SUPER} rm -f ${SYSTEMD_SERVICE_PATH}"
    log_line info "  -> 已删除: ${SYSTEMD_SERVICE_PATH}"
  else
    log_line info "  -> service 文件不存在，跳过"
  fi

  run_cmd "${SUPER} systemctl daemon-reload"
  log_line ok "systemd service 已清理"
}

# Step 3: 删除软链接
remove_symlinks() {
  log_line info "[Step 3] 删除 /usr/local/bin 下的软链接..."

  if [ -L "${SYMLINK_PATH}" ]; then
    run_cmd "${SUPER} rm -f ${SYMLINK_PATH}"
    log_line info "  -> 已删除软链接: ${SYMLINK_PATH}"
  elif [ -f "${SYMLINK_PATH}" ]; then
    log_line warn "  -> ${SYMLINK_PATH} 不是软链接而是普通文件，为安全起见删除"
    run_cmd "${SUPER} rm -f ${SYMLINK_PATH}"
  else
    log_line info "  -> 软链接不存在，跳过"
  fi

  log_line ok "软链接已清理"
}

# Step 4: 备份配置 (如需保留)
backup_config() {
  if $KEEP_CONFIG; then
    log_line info "[Step 4] 备份配置文件..."
    local backup_dir="/tmp/${PROGRAM_NAME}_config_backup_$(date '+%Y%m%d_%H%M%S')"
    if [ -d "${CONFIG_DIR}" ]; then
      run_cmd "${SUPER} cp -r ${CONFIG_DIR} ${backup_dir}"
      log_line info "  -> 配置已备份到: ${backup_dir}"
    else
      log_line info "  -> 配置目录不存在，无需备份"
    fi
  else
    log_line info "[Step 4] 不保留配置文件 (如需保留，使用 --keep-config)"
  fi
}

# Step 5: 删除 AppDir 目录 (原子化卸载的核心)
remove_appdir() {
  log_line info "[Step 5] 删除 AppDir 目录: ${INSTALL_ROOT} ..."

  if [ -d "${INSTALL_ROOT}" ]; then
    # 安全检查: 确保路径不是根目录或其他危险路径
    case "${INSTALL_ROOT}" in
      /|/usr|/usr/local|/etc|/var|/home|/root|/boot|/dev|/proc|/sys)
        log_line err "拒绝删除系统关键目录: ${INSTALL_ROOT}"
        exit 1
        ;;
    esac

    run_cmd "${SUPER} rm -rf ${INSTALL_ROOT}"
    log_line info "  -> 已删除: ${INSTALL_ROOT}"
  else
    log_line info "  -> 目录不存在: ${INSTALL_ROOT}，跳过"
  fi

  log_line ok "AppDir 目录已清理"
}

# Step 6: 清理日志目录
cleanup_logs() {
  log_line info "[Step 6] 清理日志目录: ${LOG_DIR} ..."

  if $KEEP_LOGS; then
    log_line info "  -> 保留日志目录 (--keep-logs)"
  elif [ -d "${LOG_DIR}" ]; then
    run_cmd "${SUPER} rm -rf ${LOG_DIR}"
    log_line info "  -> 已删除: ${LOG_DIR}"
  else
    log_line info "  -> 日志目录不存在，跳过"
  fi

  log_line ok "日志清理完成"
}

# ========================= 卸载完成摘要 =========================
print_summary() {
  log_line info "=============================================="
  log_line info "  卸载完成 — 摘要"
  log_line info "=============================================="
  log_line info "已删除 AppDir:     ${INSTALL_ROOT}"
  log_line info "已删除软链接:      ${SYMLINK_PATH}"
  log_line info "已删除 service:    ${SYSTEMD_SERVICE_PATH}"
  if $KEEP_LOGS; then
    log_line info "日志目录:          ${LOG_DIR} (已保留)"
  else
    log_line info "日志目录:          ${LOG_DIR} (已删除)"
  fi
  if $KEEP_CONFIG; then
    log_line info "配置备份:          /tmp/${PROGRAM_NAME}_config_backup_*"
  fi
  log_line info "卸载日志:          ${LOG_FILE}"
  log_line info "=============================================="
}

# ========================= 主流程 =========================
main() {
  parse_args "$@"
  setup_paths

  log_line info "========== my_uninstall.sh 开始执行 =========="
  log_line info "INSTALL_ROOT: ${INSTALL_ROOT}"
  log_line info "DEBUG: ${DEBUG}"
  log_line info "KEEP_LOGS: ${KEEP_LOGS}"
  log_line info "KEEP_CONFIG: ${KEEP_CONFIG}"

  need_sudo_or_die

  stop_service          # Step 1
  remove_service_file   # Step 2
  remove_symlinks       # Step 3
  backup_config         # Step 4
  remove_appdir         # Step 5
  cleanup_logs          # Step 6

  print_summary
  log_line info "========== my_uninstall.sh 执行完毕 =========="
}

main "$@"
