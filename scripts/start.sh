#!/bin/bash

# ==============================================================================
# Fast C++ Server Start Wrapper (无日志托管版)
# ==============================================================================

# --- 1. 配置区域 ---
APP_NAME="fast_cpp_server"
APP_BIN="/usr/local/bin/fast_cpp_server_dir/fast_cpp_server"
APP_HOME="/usr/local/bin"

# MQTT 配置
MQTT_ENABLED=true
MQTT_BIN="/usr/local/bin/fast_cpp_server_dir/mosquitto"
MQTT_CONF="/etc/fast_cpp_server/mosquitto.conf"

# 脚本自身的运行日志 (只记录启动、停止、重启等行为)
SCRIPT_LOG="${HOME}/startup.log"

# 动态库路径
export LD_LIBRARY_PATH=/usr/local/lib/fast_cpp_server:$LD_LIBRARY_PATH

# 定义一个不依赖外部定义的 log 函数
log_to_file() {
    local DATE=$(date '+%Y-%m-%d %H:%M:%S')
    # 打印到系统日志（可以通过 journalctl -u 查看）
    echo "[$DATE] $1"
    # 同时物理写入文件
    echo "[$DATE] $1" >> "$SCRIPT_LOG" 2>/dev/null
}

# 测试写入权限
touch "$SCRIPT_LOG" 2>/dev/null
if [ $? -ne 0 ]; then
    # 如果写不进去文件，至少让它在 systemctl status 里报错
    echo "ERROR: Cannot write to $SCRIPT_LOG. Check permissions." >&2
fi

log_to_file ">>> Systemd 正在拉起脚本..."
log_to_file "工作目录: $(pwd)"
log_to_file "程序路径: $APP_BIN"
log_to_file "MQTT 支持: $MQTT_ENABLED"
log_to_file "启动程序日志路径: $SCRIPT_LOG"

cleanup() {
    log_to_file "收到停止信号，正在终止进程..."
    if [ -n "$APP_PID" ]; then
        kill -SIGTERM "$APP_PID" 2>/dev/null
        wait "$APP_PID" 2>/dev/null
        log_to_file "主程序已停止。"
    fi
    if [ "$MQTT_ENABLED" = true ]; then
        pkill -x "mosquitto"
        log_to_file "MQTT Broker 已停止。"
    fi
    exit 0
}

trap cleanup SIGINT SIGTERM

# --- 3. 参数解析 ---
for arg in "$@"
do
    case $arg in
        --no-mqtt)
        MQTT_ENABLED=false
        shift
        ;;
    esac
done

# --- 4. 初始化 ---
cd "$APP_HOME" || exit 1
log_to_file ">>> 启动脚本开始执行..."

# --- 5. 启动 MQTT ---
start_mqtt() {
    if [ "$MQTT_ENABLED" = true ]; then
        if pgrep -x "mosquitto" > /dev/null; then
            log_to_file "MQTT Broker 已经在运行中。"
        else
            log_to_file "正在启动 MQTT Broker..."
            $MQTT_BIN -c "$MQTT_CONF" -d
            sleep 1
        fi
    fi
}

# --- 6. 主循环 (守护进程模式) ---
start_mqtt

while true; do
    log_to_file "正在启动主程序: $APP_NAME"
    
    # ---------------------------------------------------------
    # 修改点：不再进行日志重定向，直接后台启动
    # 程序内部会自动处理日志文件的生成
    # ---------------------------------------------------------
    $APP_BIN &
    
    APP_PID=$!
    log_to_file "主程序已启动 (PID: $APP_PID)"

    # 阻塞等待进程结束
    wait "$APP_PID"
    EXIT_CODE=$?

    log_to_file "主程序异常退出 (Exit Code: $EXIT_CODE)，3秒后尝试重启..."
    
    # 检查 MQTT 状态，防止它也挂了
    start_mqtt
    
    sleep 3
done