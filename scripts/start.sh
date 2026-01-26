#!/bin/bash
set -euo pipefail

APP_NAME="fast_cpp_server"
PROGRAM_NAME="fast_cpp_server"

MODE="system"
DEBUG=false
MQTT_ENABLED=true

# system
SYS_BIN_DIR="/usr/local/bin/fast_cpp_server_dir"
SYS_LIB_DIR="/usr/local/lib/fast_cpp_server"
SYS_CONFIG_DIR="/etc/fast_cpp_server"
APP_BIN_SYS="${SYS_BIN_DIR}/fast_cpp_server"
MQTT_BIN_SYS="${SYS_BIN_DIR}/mosquitto"
MQTT_CONF_SYS="${SYS_CONFIG_DIR}/mosquitto.conf"

# user
USER_PREFIX="${HOME}/.local/${PROGRAM_NAME}"
USER_BIN_DIR="${USER_PREFIX}/bin"
USER_LIB_DIR="${USER_PREFIX}/lib"
USER_CONFIG_DIR="${HOME}/.config/${PROGRAM_NAME}"
USER_DATA_ROOT="${HOME}/.local/share/${PROGRAM_NAME}"
USER_LOG_DIR="${USER_DATA_ROOT}/logs"
USER_RUN_DIR="${USER_DATA_ROOT}/run"
USER_DATA_DIR="${USER_DATA_ROOT}/data"
APP_BIN_USER="${USER_BIN_DIR}/fast_cpp_server"
MQTT_BIN_USER="${USER_BIN_DIR}/mosquitto"
MQTT_CONF_USER="${USER_CONFIG_DIR}/mosquitto.conf"

# logging
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="${SCRIPT_DIR}/logs"
TS="$(date '+%Y%m%d_%H%M%S')"
LOG_FILE="${LOG_DIR}/start_${TS}.log"

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
    bash -c "${cmd}" >> "${LOG_FILE}" 2>&1 || true
    return 0
  fi
}

for arg in "$@"; do
  case "$arg" in
    --debug) DEBUG=true ;;
    --no-mqtt) MQTT_ENABLED=false ;;
    --user) MODE="user" ;;
    --system) MODE="system" ;;
  esac
done

log_line info "Start begin: MODE=${MODE}, DEBUG=${DEBUG}, MQTT_ENABLED=${MQTT_ENABLED}"
log_line info "Log file: ${LOG_FILE}"

if [ "${MODE}" == "user" ]; then
  APP_BIN="${APP_BIN_USER}"
  MQTT_BIN="${MQTT_BIN_USER}"
  MQTT_CONF="${MQTT_CONF_USER}"
  export LD_LIBRARY_PATH="${USER_LIB_DIR}:${LD_LIBRARY_PATH:-}"
  export PATH="${USER_BIN_DIR}:${PATH}"
  mkdir -p "${USER_LOG_DIR}" "${USER_RUN_DIR}" "${USER_DATA_DIR}"
else
  APP_BIN="${APP_BIN_SYS}"
  MQTT_BIN="${MQTT_BIN_SYS}"
  MQTT_CONF="${MQTT_CONF_SYS}"
  export LD_LIBRARY_PATH="${SYS_LIB_DIR}:${LD_LIBRARY_PATH:-}"
fi

log_line info "APP_BIN=${APP_BIN}"
log_line info "MQTT_BIN=${MQTT_BIN}"
log_line info "MQTT_CONF=${MQTT_CONF}"
log_line info "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"

cleanup() {
  log_line warn "Received stop signal, cleaning up..."
  if [ -n "${APP_PID:-}" ]; then
    run_cmd "kill -SIGTERM ${APP_PID} 2>/dev/null || true"
    run_cmd "wait ${APP_PID} 2>/dev/null || true"
  fi
  if [ "${MQTT_ENABLED}" = true ]; then
    run_cmd "pkill -x mosquitto 2>/dev/null || true"
  fi
  log_line info "Exit."
  exit 0
}
trap cleanup SIGINT SIGTERM

start_mqtt() {
  if [ "${MQTT_ENABLED}" = true ]; then
    run_cmd "pgrep -x mosquitto >/dev/null 2>&1 && echo 'mosquitto already running' || true"
    run_cmd "\"${MQTT_BIN}\" -c \"${MQTT_CONF}\" -d || true"
    run_cmd "sleep 1"
  fi
}

start_mqtt

while true; do
  log_line info "Launching ${APP_NAME}..."
  if $DEBUG; then
    log_line dev "\"${APP_BIN}\" &"
    log_line dev "APP_PID=<dry-run>"
    log_line dev "wait <dry-run>"
    log_line dev "sleep 3"
    break
  else
    "${APP_BIN}" >> "${LOG_FILE}" 2>&1 &
    APP_PID=$!
    log_line info "APP started PID=${APP_PID}"
    wait "${APP_PID}" || true
    EXIT_CODE=$?
    log_line warn "APP exited code=${EXIT_CODE}, restart in 3s..."
    start_mqtt
    sleep 3
  fi
done