#!/bin/bash

PROGRAM_NAME="fast_cpp_server"
SERVICE_FILE_NAME=${PROGRAM_NAME}.service
TARGET_SERVICE_FILE_PATH=/etc/systemd/system/${SERVICE_FILE_NAME}
BIN_PATH="/usr/local/bin/${PROGRAM_NAME}"
CONFIG_PATH="/etc/${PROGRAM_NAME}"
LIB_PATH="/usr/local/lib/${PROGRAM_NAME}"
LOG_PATH="/var/${PROGRAM_NAME}"
TEMP_DIR="/tmp/${PROGRAM_NAME}"
SHARE_DIR="/usr/share/${PROGRAM_NAME}"

SUPER="sudo"
SUPER=""

# Check if DEBUG mode is enabled
DEBUG=false
if [ "$1" == "--debug" ]; then
    DEBUG=true
    echo "🔧 DEBUG mode enabled. No changes will be made."
else
    echo "🔧 DEBUG mode disabled. Changes will be made."
fi

# Function to execute commands or print them in DEBUG mode
execute() {
    if $DEBUG; then
        echo "DEBUG: $*"
    else
        eval "$@"
    fi
}

echo "Uninstalling ${PROGRAM_NAME}..."

# Stop the service
echo "🛑 Stopping service ${SERVICE_FILE_NAME}..."
if [ -f "${TARGET_SERVICE_FILE_PATH}" ]; then
    
    execute "${SUPER}" systemctl stop "${SERVICE_FILE_NAME}"
    execute "${SUPER}" systemctl disable "${SERVICE_FILE_NAME}"
else
    echo "Executing: "${SUPER}" systemctl stop ${SERVICE_FILE_NAME}"
    echo "⚠️ Service file ${TARGET_SERVICE_FILE_PATH} does not exist. Skipping stop and disable."
fi

# Delete binary file
echo "🗑️ Deleting binary file at ${BIN_PATH}..."
if [ -f "${BIN_PATH}" ]; then
    execute "${SUPER}" rm -f "${BIN_PATH}"
    echo "✅ Binary file deleted."
else
    echo "⚠️ Binary file ${BIN_PATH} does not exist. Skipping deletion."
fi

# Delete config files and folder
echo "🗑️ Deleting config files and folder at ${CONFIG_PATH}..."
if [ -d "${CONFIG_PATH}" ]; then
    execute "${SUPER}" rm -rf "${CONFIG_PATH}"
    echo "✅ Config files and folder deleted."
else
    echo "⚠️ Config folder ${CONFIG_PATH} does not exist. Skipping deletion."
fi

# Delete library files and folder
echo "🗑️ Deleting library files and folder at ${LIB_PATH}..."
if [ -d "${LIB_PATH}" ]; then
    execute "${SUPER}" rm -rf "${LIB_PATH}"
    echo "✅ Library files and folder deleted."
else
    echo "⚠️ Library folder ${LIB_PATH} does not exist. Skipping deletion."
fi

# Delete log files and folder
echo "🗑️ Deleting log files and folder at ${LOG_PATH}..."
if [ -d "${LOG_PATH}" ]; then
    execute "${SUPER}" rm -rf "${LOG_PATH}"
    echo "✅ Log files and folder deleted."
else
    echo "⚠️ Log folder ${LOG_PATH} does not exist. Skipping deletion."
fi

# delete tmp folder
echo "🗑️ Deleting temp folder at ${TEMP_DIR}..."
if [ -d "${TEMP_DIR}" ]; then
    execute "${SUPER}" rm -rf "${TEMP_DIR}"
    echo "✅ Temp folder deleted."
else
    echo "⚠️ Temp folder ${TEMP_DIR} does not exist. Skipping deletion."
fi

# delete share folder
echo "🗑️ Deleting share folder at ${SHARE_DIR}..."
if [ -d "${SHARE_DIR}" ]; then
    execute "${SUPER}" rm -rf "${SHARE_DIR}"
    echo "✅ Share folder deleted."
else
    echo "⚠️ Share folder ${SHARE_DIR} does not exist. Skipping deletion."
fi

# Delete service file
echo "🗑️ Deleting service file at ${TARGET_SERVICE_FILE_PATH}..."
if [ -f "${TARGET_SERVICE_FILE_PATH}" ]; then
    execute "${SUPER}" rm -f "${TARGET_SERVICE_FILE_PATH}"
    echo "✅ Service file deleted."
else
    echo "⚠️ Service file ${TARGET_SERVICE_FILE_PATH} does not exist. Skipping deletion."
fi

# Reload systemd daemon
echo "🔄 Reloading systemd daemon..."
execute "${SUPER}" systemctl daemon-reload

echo "🍺 Uninstallation of ${PROGRAM_NAME} completed!"


echo ""
echo "-----------------------------------------------------------------"
echo "🔍 Installation details:"
echo "-----------------------------------------------------------------"
echo "       bin path: ${BIN_PATH}"
echo "       lib path: ${LIB_PATH}"
echo "    config path: ${CONFIG_PATH}"
echo "       log path: ${LOG_PATH}"
echo "   service path: ${TARGET_SERVICE_FILE_PATH}"
echo "      temp path: ${TEMP_DIR}"
# echo " service status: $(sudo systemctl status ${PROGRAM_NAME}.service | grep Active)"
# echo "    service log: $(sudo journalctl -u ${PROGRAM_NAME}.service --no-pager | tail -n 10)"
echo "-----------------------------------------------------------------"
echo ">>> "${SUPER}" systemctl status ${PROGRAM_NAME}.service"
execute "${SUPER}" systemctl status "${PROGRAM_NAME}.service"