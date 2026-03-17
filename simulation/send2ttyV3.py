import serial
import time

# 指向 socat 创建的其中一个端点
ser = serial.Serial('/tmp/ttyV3_SIM', 115200)

def send_heartbeat():
    # 构造一个简易的飞控状态报文 (0x01)
    # 帧头 EB 90 | CNT 00 | 类型 01 | 模式 00 | 星数 0A ...
    payload = bytearray([
        0xEB, 0x90, # 帧头
        0x01,       # CNT
        0x01,       # 帧类型 (状态)
        0x00,       # 运行模式
        0x0A,       # 星数 (10颗)
    ])
    
    # 补充 89 字节中剩余的部分 (用0填充)
    payload.extend([0] * (87 - len(payload)))
    
    # 计算和校验 (Sum of 0~N-1, low 8 bits)
    checksum = sum(payload) & 0xFF
    payload.append(checksum)
    
    # 帧尾 ED DE
    payload.extend([0xED, 0xDE])
    
    ser.write(payload)
    print(f"Sent Heartbeat: {payload.hex(' ')}")

while True:
    send_heartbeat()
    time.sleep(0.5) # 500ms 发送频率