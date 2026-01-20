#!/bin/bash

PROGRAM_NAME="fast_cpp_server"
INSTALL_PATH="/usr/local"
BIN_PATH="${INSTALL_PATH}/bin"
LIB_PATH="${INSTALL_PATH}/lib/${PROGRAM_NAME}"
CONFIG_PATH="/etc/${PROGRAM_NAME}"
LOG_PATH="/var/${PROGRAM_NAME}/logs"
TEMP_DIR="/tmp/${PROGRAM_NAME}"
SHARE_DIR="/usr/share/${PROGRAM_NAME}"
SERVICE_PATH="/etc/systemd/system"

SUPER="sudo"

# Check if DEBUG mode is enabled
DEBUG=false
if [ "$1" == "--debug" ]; then
    DEBUG=true
    echo "🔧 DEBUG mode enabled. No changes will be made."
else
    echo "🔧 DEBUG mode disabled. Changes will be made."
fi

echo "Installing ${PROGRAM_NAME}..."

# Function to execute commands or print them in DEBUG mode
execute() {
    if $DEBUG; then
        echo "DEBUG: $*"
    else
        eval "$@"
    fi
}

# 停止并禁用已有服务
echo "🛑 Stopping and disabling existing ${PROGRAM_NAME} service if any..."
echo "🔧 ${SUPER} systemctl stop ${PROGRAM_NAME}.service 2>/dev/null"
execute "${SUPER}" systemctl stop "${PROGRAM_NAME}.service" 2>/dev/null
echo "🔧 ---------------------------------------------------------------"

# Check and create log directory
echo "📁 Checking log directory at ${LOG_PATH}..."
if [ -d "${LOG_PATH}" ]; then
    echo "✅ Log directory already exists."
else
    echo "📁 Creating log directory..."
    execute "${SUPER}" mkdir -p "${LOG_PATH}"
    execute "${SUPER}" chmod 755 "${LOG_PATH}"
fi

# Check and create config directory, then copy config file
echo "📁 Checking config directory at ${CONFIG_PATH}..."
if [ -d "${CONFIG_PATH}" ]; then
    echo "✅ Config directory already exists."
else
    echo "📁 Creating config directory..."
    execute "${SUPER}" mkdir -p "${CONFIG_PATH}"
fi

echo "📄 Copying config file to ${CONFIG_PATH}..."
execute "${SUPER}" cp -r ./config/* "${CONFIG_PATH}/"
execute "${SUPER}" chmod 644 "${CONFIG_PATH}/*"

# Check and create library directory, then copy library files or folders
echo "📁 Checking library directory at ${LIB_PATH}..."
if [ -d "${LIB_PATH}" ]; then
    echo "✅ Library directory already exists."
else
    echo "📁 Creating library directory..."
    execute "${SUPER}" mkdir -p "${LIB_PATH}"
fi

echo "📄 Copying library files or folders to ${LIB_PATH}..."
execute "${SUPER}" cp -r ./lib/* "${LIB_PATH}/"
execute "${SUPER}" chmod 644 "${LIB_PATH}/*"

# Copy binary file to /usr/local/bin
echo "📁 Checking binary directory at ${BIN_PATH}..."
if [ -d "${BIN_PATH}" ]; then
    echo "✅ Binary directory already exists."
else
    echo "📁 Creating binary directory..."
    execute "${SUPER}" mkdir -p "${BIN_PATH}"
fi

echo "📄 Copying binary file to ${BIN_PATH}..."
if [ -f "${BIN_PATH}/${PROGRAM_NAME}" ]; then
    echo "⚠️ Binary file: ${BIN_PATH}/${PROGRAM_NAME} already exists. Replacing it..."
    echo "🔧 ${SUPER} rm -f ${BIN_PATH}/${PROGRAM_NAME}"
    ${SUPER} rm -f "${BIN_PATH}/${PROGRAM_NAME}"
fi
execute "${SUPER}" cp ./bin/${PROGRAM_NAME} "${BIN_PATH}/${PROGRAM_NAME}"
execute "${SUPER}" chmod 755 "${BIN_PATH}/${PROGRAM_NAME}"

# Copy temp folder to /tmp/${PROGRAM_NAME}
echo "📁 Checking temp directory at ${TEMP_DIR}..."
if [ -d "${TEMP_DIR}" ]; then
    echo "✅ Temp directory already exists."
else
    echo "📁 Creating temp directory..."
    execute "${SUPER}" mkdir -p "${TEMP_DIR}"
fi

#Copy share folder to /usr/share/${PROGRAM_NAME}
echo "📁 Checking share directory at ${SHARE_DIR}..."
if [ -d "${SHARE_DIR}" ]; then
    echo "✅ Share directory already exists."
else
    echo "📁 Creating share directory..."
    execute "${SUPER}" mkdir -p "${SHARE_DIR}"
fi
echo "📄 Copying share files to ${SHARE_DIR}..."
execute "${SUPER}" cp -r ./swagger-res "${SHARE_DIR}/"

# Copy service file to /etc/systemd/system
echo "📁 Checking service file at ${SERVICE_PATH}..."
if [ -f "${SERVICE_PATH}/${PROGRAM_NAME}.service" ]; then
    echo "⚠️ Service file:${SERVICE_PATH}/${PROGRAM_NAME}.service already exists. Replacing it..."
fi
execute "${SUPER}" cp ./service/${PROGRAM_NAME}.service "${SERVICE_PATH}/${PROGRAM_NAME}.service"
execute "${SUPER}" chmod 777 "${SERVICE_PATH}/${PROGRAM_NAME}.service"


# Reload systemd configuration
echo "🔄 Reloading systemd daemon..."
execute "${SUPER}" systemctl daemon-reload

# Enable and start the service
echo "🚀 Enabling and starting ${PROGRAM_NAME} service..."
execute "${SUPER}" systemctl enable "${PROGRAM_NAME}.service"
execute "${SUPER}" systemctl start "${PROGRAM_NAME}.service"

echo "🍺 Installation complete!"

echo ""
echo "-----------------------------------------------------------------"
echo "🔍 Installation details:"
echo "-----------------------------------------------------------------"
echo "       bin path: ${BIN_PATH}/${PROGRAM_NAME}"
echo "       lib path: ${LIB_PATH}"
echo "    config path: ${CONFIG_PATH}"
echo "       log path: ${LOG_PATH}"
echo "   service path: ${SERVICE_PATH}"
echo "   service name: ${PROGRAM_NAME}.service"
echo "      temp path: ${TEMP_DIR}"
echo "     share path: ${SHARE_DIR}"
# echo " service status: $(sudo systemctl status ${PROGRAM_NAME}.service | grep Active)"
# echo "    service log: $(sudo journalctl -u ${PROGRAM_NAME}.service --no-pager | tail -n 10)"
echo "-----------------------------------------------------------------"
echo ">>> "${SUPER}" systemctl status ${PROGRAM_NAME}.service"
execute "${SUPER}" systemctl status "${PROGRAM_NAME}.service"