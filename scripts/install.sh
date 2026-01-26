#!/bin/bash
set -euo pipefail

PROGRAM_NAME="fast_cpp_server"
MQTT_PROGRAM_NAME="mosquitto"
START_SCRIPT_NAME="start.sh"

MODE="system"   # system | user
DEBUG=false     # --debug => dry-run

# -------- system paths (need root) --------
INSTALL_PATH="/usr/local"
BIN_FOLDER_PATH="/usr/local/bin/${PROGRAM_NAME}_dir"
LIB_PATH="${INSTALL_PATH}/lib/${PROGRAM_NAME}"
CONFIG_PATH="/etc/${PROGRAM_NAME}"
LOG_PATH="/var/${PROGRAM_NAME}/logs"
TEMP_DIR="/tmp/${PROGRAM_NAME}"
SHARE_DIR="/usr/share/${PROGRAM_NAME}"
SERVICE_PATH="/etc/systemd/system"

# -------- user paths (no root) --------
USER_PREFIX="${HOME}/.local/${PROGRAM_NAME}"
USER_BIN_FOLDER_PATH="${USER_PREFIX}/bin"
USER_LIB_PATH="${USER_PREFIX}/lib"
USER_CONFIG_PATH="${HOME}/.config/${PROGRAM_NAME}"
USER_LOG_PATH="${HOME}/.local/share/${PROGRAM_NAME}/logs"
USER_DATA_PATH="${HOME}/.local/share/${PROGRAM_NAME}/data"
USER_TEMP_DIR="${HOME}/.cache/${PROGRAM_NAME}"
USER_SHARE_DIR="${USER_PREFIX}/share"

SUPER="sudo"

# -------- logging --------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="${SCRIPT_DIR}/logs"
TS="$(date '+%Y%m%d_%H%M%S')"
LOG_FILE="${LOG_DIR}/install_${TS}.log"

# colors
C_RESET="\033[0m"
C_GREEN="\033[32m"
C_YELLOW="\033[33m"
C_RED="\033[31m"
C_MAGENTA="\033[35m"
C_CYAN="\033[36m"

tag() {
  case "${1:-}" in
    dev)  echo -e "${C_MAGENTA}[dev]${C_RESET}" ;;
    pro)  echo -e "${C_GREEN}[pro]${C_RESET}" ;;
    info) echo -e "${C_CYAN}[info]${C_RESET}" ;;
    warn) echo -e "${C_YELLOW}[warn]${C_RESET}" ;;
    err)  echo -e "${C_RED}[err]${C_RESET}" ;;
    *)    echo -e "[log]" ;;
  esac
}

log_line() {
  local level="$1"; shift
  local msg="$*"
  mkdir -p "${LOG_DIR}" >/dev/null 2>&1 || true
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] [${level}] ${msg}" >> "${LOG_FILE}" 2>/dev/null || true
  echo -e "$(tag "${level}") ${msg}"
}

run_cmd() {
  local cmd="$*"
  if $DEBUG; then
    log_line dev "${cmd}"
    return 0
  else
    log_line pro "${cmd}"
    set +e
    bash -c "${cmd}" >> "${LOG_FILE}" 2>&1
    local rc=$?
    set -e
    if [ $rc -ne 0 ]; then
      log_line err "Command failed (rc=${rc}): ${cmd}"
      log_line err "See log: ${LOG_FILE}"
      exit $rc
    fi
    return 0
  fi
}

need_sudo_or_die() {
  if $DEBUG; then return 0; fi
  if ! sudo -n true 2>/dev/null; then
    log_line err "sudo is not available (possibly no_new_privileges)."
    log_line err "Use: ./install.sh --user"
    exit 1
  fi
}

# -------- parse args --------
for arg in "$@"; do
  case "$arg" in
    --debug) DEBUG=true ;;
    --user) MODE="user" ;;
    --system) MODE="system" ;;
  esac
done

log_line info "Install begin: PROGRAM_NAME=${PROGRAM_NAME}, MODE=${MODE}, DEBUG=${DEBUG}"
log_line info "Release dir: ${SCRIPT_DIR}"
log_line info "Log file: ${LOG_FILE}"

# -------- main --------
if [ "${MODE}" == "system" ]; then
  need_sudo_or_die

  log_line info "Stopping existing service if any..."
  run_cmd "${SUPER} systemctl stop ${PROGRAM_NAME}.service 2>/dev/null || true"
  run_cmd "${SUPER} systemctl disable ${PROGRAM_NAME}.service 2>/dev/null || true"

  log_line info "Prepare log dir: ${LOG_PATH}"
  run_cmd "${SUPER} mkdir -p ${LOG_PATH}"
  run_cmd "${SUPER} chmod 777 ${LOG_PATH} || true"

  log_line info "Prepare config dir: ${CONFIG_PATH}"
  run_cmd "${SUPER} mkdir -p ${CONFIG_PATH}"

  log_line info "Install configs: ./config/* -> ${CONFIG_PATH}/"
  run_cmd "${SUPER} cp -r ./config/* ${CONFIG_PATH}/"
  run_cmd "${SUPER} chmod 644 ${CONFIG_PATH}/* || true"

  # Ensure /etc/fast_cpp_server/config.ini exists (program reads config.ini)
  if [ -f "./config/config.system.ini" ]; then
    log_line info "Ensure system config.ini: ./config/config.system.ini -> ${CONFIG_PATH}/config.ini"
    run_cmd "${SUPER} cp ./config/config.system.ini ${CONFIG_PATH}/config.ini"
    run_cmd "${SUPER} chmod 644 ${CONFIG_PATH}/config.ini || true"
  else
    log_line warn "Missing ./config/config.system.ini (skip generating ${CONFIG_PATH}/config.ini)"
  fi

  log_line info "Prepare lib dir: ${LIB_PATH}"
  run_cmd "${SUPER} mkdir -p ${LIB_PATH}"
  log_line info "Install libs: ./lib/* -> ${LIB_PATH}/"
  run_cmd "${SUPER} cp -r ./lib/* ${LIB_PATH}/"
  run_cmd "${SUPER} chmod 644 ${LIB_PATH}/* || true"

  log_line info "Prepare bin dir: ${BIN_FOLDER_PATH}"
  run_cmd "${SUPER} mkdir -p ${BIN_FOLDER_PATH}"
  log_line info "Install bins: ./bin/${PROGRAM_NAME}, ./bin/${MQTT_PROGRAM_NAME}, ./${START_SCRIPT_NAME}"
  run_cmd "${SUPER} cp ./bin/${PROGRAM_NAME}      ${BIN_FOLDER_PATH}/${PROGRAM_NAME}"
  run_cmd "${SUPER} cp ./bin/${MQTT_PROGRAM_NAME} ${BIN_FOLDER_PATH}/${MQTT_PROGRAM_NAME}"
  run_cmd "${SUPER} cp ./${START_SCRIPT_NAME}     ${BIN_FOLDER_PATH}/${START_SCRIPT_NAME}"
  run_cmd "${SUPER} chmod 755 ${BIN_FOLDER_PATH}/${PROGRAM_NAME}"
  run_cmd "${SUPER} chmod 755 ${BIN_FOLDER_PATH}/${MQTT_PROGRAM_NAME}"
  run_cmd "${SUPER} chmod 755 ${BIN_FOLDER_PATH}/${START_SCRIPT_NAME}"

  log_line info "Prepare temp dir: ${TEMP_DIR}"
  run_cmd "${SUPER} mkdir -p ${TEMP_DIR}"

  log_line info "Prepare share dir: ${SHARE_DIR}"
  run_cmd "${SUPER} mkdir -p ${SHARE_DIR}"
  log_line info "Install swagger-res -> ${SHARE_DIR}/"
  run_cmd "${SUPER} cp -r ./swagger-res ${SHARE_DIR}/"

  log_line info "Install systemd service: ./service/${PROGRAM_NAME}.service -> ${SERVICE_PATH}/"
  run_cmd "${SUPER} cp ./service/${PROGRAM_NAME}.service ${SERVICE_PATH}/${PROGRAM_NAME}.service"
  run_cmd "${SUPER} chmod 644 ${SERVICE_PATH}/${PROGRAM_NAME}.service"

  log_line info "Reload systemd daemon"
  run_cmd "${SUPER} systemctl daemon-reload"

  log_line info "Enable and start service"
  run_cmd "${SUPER} systemctl enable ${PROGRAM_NAME}.service"
  run_cmd "${SUPER} systemctl start ${PROGRAM_NAME}.service"

  log_line info "Install done (system)."
  log_line info "bin: ${BIN_FOLDER_PATH}/${PROGRAM_NAME}"
  log_line info "mqtt: ${BIN_FOLDER_PATH}/${MQTT_PROGRAM_NAME}"
  log_line info "cfg: ${CONFIG_PATH}/config.ini"
  log_line info "lib: ${LIB_PATH}"
  log_line info "log: ${LOG_PATH}"
  log_line info "service: ${SERVICE_PATH}/${PROGRAM_NAME}.service"
  log_line info "Check: ${SUPER} systemctl status ${PROGRAM_NAME}.service"

  if ! $DEBUG; then
    set +e
    ${SUPER} systemctl status "${PROGRAM_NAME}.service" >> "${LOG_FILE}" 2>&1
    set -e
  fi

else
  log_line info "User install: no sudo, no systemd."
  log_line info "prefix: ${USER_PREFIX}"

  run_cmd "mkdir -p ${USER_BIN_FOLDER_PATH} ${USER_LIB_PATH} ${USER_SHARE_DIR}"
  run_cmd "mkdir -p ${USER_CONFIG_PATH} ${USER_LOG_PATH} ${USER_DATA_PATH} ${USER_TEMP_DIR}"

  log_line info "Install bins -> ${USER_BIN_FOLDER_PATH}/"
  run_cmd "cp ./bin/${PROGRAM_NAME}      ${USER_BIN_FOLDER_PATH}/${PROGRAM_NAME}"
  run_cmd "cp ./bin/${MQTT_PROGRAM_NAME} ${USER_BIN_FOLDER_PATH}/${MQTT_PROGRAM_NAME}"
  run_cmd "cp ./${START_SCRIPT_NAME}     ${USER_BIN_FOLDER_PATH}/${START_SCRIPT_NAME}"
  run_cmd "chmod 755 ${USER_BIN_FOLDER_PATH}/${PROGRAM_NAME}"
  run_cmd "chmod 755 ${USER_BIN_FOLDER_PATH}/${MQTT_PROGRAM_NAME}"
  run_cmd "chmod 755 ${USER_BIN_FOLDER_PATH}/${START_SCRIPT_NAME}"

  log_line info "Install libs -> ${USER_LIB_PATH}/"
  run_cmd "cp -r ./lib/* ${USER_LIB_PATH}/"
  run_cmd "chmod 644 ${USER_LIB_PATH}/* || true"

  log_line info "Install swagger-res -> ${USER_SHARE_DIR}/"
  run_cmd "cp -r ./swagger-res ${USER_SHARE_DIR}/"

  # configs: non-overwrite
  log_line info "Install configs (non-overwrite) -> ${USER_CONFIG_PATH}/"
  if $DEBUG; then
    run_cmd "for f in ./config/*; do echo \"would copy \$f -> ${USER_CONFIG_PATH}/\"; done"
  else
    for f in ./config/*; do
      base="$(basename "$f")"
      if [ ! -f "${USER_CONFIG_PATH}/${base}" ]; then
        log_line pro "cp ${f} ${USER_CONFIG_PATH}/${base}"
        cp "$f" "${USER_CONFIG_PATH}/${base}" >> "${LOG_FILE}" 2>&1
        chmod 644 "${USER_CONFIG_PATH}/${base}" >> "${LOG_FILE}" 2>&1 || true
      else
        log_line info "Keep existing config: ${USER_CONFIG_PATH}/${base}"
      fi
    done
  fi

  # Generate runtime config.ini from template (program reads config.ini)
  TEMPLATE="./config/config.user.template.ini"
  OUT_CFG="${USER_CONFIG_PATH}/config.ini"
  log_line info "Generate user runtime config.ini -> ${OUT_CFG}"
  if [ -f "${TEMPLATE}" ]; then
    if $DEBUG; then
      run_cmd "sed \"s|\\\${HOME}|${HOME}|g\" ${TEMPLATE} > ${OUT_CFG}"
    else
      sed "s|\${HOME}|${HOME}|g" "${TEMPLATE}" > "${OUT_CFG}"
      chmod 644 "${OUT_CFG}" || true
      log_line pro "Generated: ${OUT_CFG}"
    fi
  else
    log_line warn "Missing template: ${TEMPLATE} (skip generating ${OUT_CFG})"
  fi

  log_line info "Install done (user)."
  log_line info "Run: ${USER_BIN_FOLDER_PATH}/${START_SCRIPT_NAME} --user"
  log_line info "Config: ${USER_CONFIG_PATH}/config.ini"
  log_line info "Logs: ${USER_LOG_PATH}"
  log_line info "Data: ${USER_DATA_PATH}"
fi

log_line info "Install finished successfully."