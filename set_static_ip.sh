#!/usr/bin/env bash

set -e

LOG_FILE="/tmp/set_static_ip.log"

# ===== 默认参数 =====
IP=""
MASK="24"
GW=""
DNS=""
IFACE=""
ACTION="$1"

shift || true

# ===== 日志函数 =====
log() {
    echo -e "[INFO] $1" | tee -a $LOG_FILE
}

warn() {
    echo -e "[WARN] $1" | tee -a $LOG_FILE
}

err() {
    echo -e "[ERROR] $1" | tee -a $LOG_FILE
    exit 1
}

# ===== 参数解析 =====
while [[ $# -gt 0 ]]; do
    case "$1" in
        --ip) IP="$2"; shift 2 ;;
        --mask) MASK="$2"; shift 2 ;;
        --gw) GW="$2"; shift 2 ;;
        --dns) DNS="$2"; shift 2 ;;
        --iface) IFACE="$2"; shift 2 ;;
        *) err "未知参数: $1" ;;
    esac
done

# ===== 检测网卡 =====
detect_iface() {
    if [[ -n "$IFACE" ]]; then
        log "使用指定网卡: $IFACE"
        return
    fi

    IFACE=$(ip route | grep default | awk '{print $5}' | head -n1)

    if [[ -z "$IFACE" ]]; then
        warn "未检测到默认网卡，尝试选第一个 UP 网卡"
        IFACE=$(ip -o link show | awk -F': ' '{print $2}' | grep -v lo | head -n1)
    fi

    log "检测到网卡: $IFACE"
}

# ===== 状态 =====
status() {
    log "===== 当前网络状态 ====="
    ip addr show $IFACE
    echo ""
    ip route
}

# ===== 新增IP（不修改原配置）=====
add_ip() {
    if [[ -z "$IP" ]]; then
        err "必须提供 --ip"
    fi

    log "新增IP: $IP/$MASK 到 $IFACE"

    sudo ip addr add $IP/$MASK dev $IFACE || warn "IP可能已存在"

    log "当前IP状态："
    ip addr show $IFACE
}

# ===== 永久写入（interfaces）=====
apply_interfaces() {
    FILE="/etc/network/interfaces"

    if [[ -z "$IP" || -z "$GW" ]]; then
        err "apply 模式必须提供 --ip 和 --gw"
    fi

    log "备份配置文件"
    sudo cp $FILE ${FILE}.bak.$(date +%s)

    log "写入静态IP配置"

    sudo tee $FILE > /dev/null <<EOF
auto $IFACE
iface $IFACE inet static
    address $IP
    netmask 255.255.255.0
    gateway $GW
    dns-nameservers ${DNS//,/ }
EOF

    log "重启网络"
    sudo ifdown $IFACE || true
    sudo ifup $IFACE

    status
}

# ===== 主流程 =====

if [[ -z "$ACTION" ]]; then
    err "用法: $0 [check|status|add|apply]"
fi

detect_iface

case "$ACTION" in
    check|status)
        status
        ;;
    add)
        add_ip
        ;;
    apply)
        read -p "⚠️ 将覆盖当前IP配置，确认? (yes/no): " confirm
        if [[ "$confirm" != "yes" ]]; then
            warn "已取消"
            exit 0
        fi
        apply_interfaces
        ;;
    *)
        err "未知操作: $ACTION"
        ;;
esac