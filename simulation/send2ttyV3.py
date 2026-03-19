#!/usr/bin/env python3

import argparse
import grp
import os
import pwd
import select
import shutil
import signal
import stat
import struct
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path


FRAME_HEADER = bytes((0xEB, 0x90))
FRAME_TAIL = bytes((0xED, 0xDE))

FRAME_TYPE_HEARTBEAT = 0x01
FRAME_TYPE_SET_DESTINATION = 0x02
FRAME_TYPE_COMMAND = 0x10
FRAME_TYPE_REPLY = 0x20
FRAME_TYPE_GIMBAL_CONTROL = 0x30

CMD_SET_DESTINATION = 0x01
CMD_SET_ROUTE = 0x02
CMD_SET_ANGLE = 0x03
CMD_SET_SPEED = 0x04
CMD_SET_ALTITUDE = 0x05
CMD_POWER_SWITCH = 0x06
CMD_PARACHUTE = 0x07
CMD_BUTTON = 0x09
CMD_SET_ORIGIN_RETURN = 0x0A
CMD_SET_GEOFENCE = 0x0B
CMD_SWITCH_MODE = 0x0C
CMD_GUIDANCE = 0x0D
CMD_GUIDANCE_NEW = 0x0E
CMD_GIMBAL_ANG_RATE = 0x0F
CMD_GIMBAL_ANGLE = 0x10
CMD_TARGET_STATE = 0x11

HEARTBEAT_PAYLOAD_FORMAT = "<BBBiiHHHHhhhhhhhhHBHHBHHQHHHHHHHHBBBHHI"
HEARTBEAT_PAYLOAD_LEN = struct.calcsize(HEARTBEAT_PAYLOAD_FORMAT)
HEARTBEAT_FRAME_LEN = 89

if HEARTBEAT_PAYLOAD_LEN != 82:
    raise RuntimeError(f"heartbeat payload length mismatch: {HEARTBEAT_PAYLOAD_LEN}")


@dataclass
class ParsedFrame:
    cnt: int
    frame_type: int
    payload: bytes
    checksum: int
    valid: bool
    raw: bytes


@dataclass
class HeartbeatState:
    aircraft_id: int = 1
    run_mode: int = 1
    satellite_count: int = 12
    longitude: int = 1163974280
    latitude: int = 399084200
    altitude: int = 1234
    relative_alt: int = 560
    airspeed: int = 1530
    groundspeed: int = 1480
    velocity_x: int = 122
    velocity_y: int = -35
    velocity_z: int = -5
    accel_x: int = 3
    accel_y: int = -2
    accel_z: int = -1
    roll: int = 150
    pitch: int = -80
    yaw: int = 9000
    flight_mode: int = 0x03
    current_waypoint: int = 2
    battery_voltage: int = 2400
    battery_percent: int = 86
    battery_current: int = 55
    atm_pressure: int = 10132
    fault_info: int = 0
    rotation_speed: int = 120
    throttle: int = 640
    target_altitude: int = 1200
    target_speed: int = 1500
    origin_distance: int = 80
    origin_heading: int = 27000
    target_distance: int = 520
    flight_state: int = 0x03
    altitude_state: int = 0x01
    state_switch_src: int = 0x01
    reserved: int = 0


def log(message: str) -> None:
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] {message}", flush=True)


def format_hex(data: bytes, max_bytes: int = 96) -> str:
    if not data:
        return "<empty>"
    view = data[:max_bytes]
    text = " ".join(f"{item:02X}" for item in view)
    if len(data) > max_bytes:
        text += f" ...( +{len(data) - max_bytes} bytes)"
    return text


def calc_checksum(data: bytes) -> int:
    return sum(data) & 0xFF


def build_frame(cnt: int, frame_type: int, payload: bytes) -> bytes:
    frame = bytearray(FRAME_HEADER)
    frame.append(cnt & 0xFF)
    frame.append(frame_type & 0xFF)
    frame.extend(payload)
    frame.append(calc_checksum(frame))
    frame.extend(FRAME_TAIL)
    return bytes(frame)


def build_heartbeat_payload(state: HeartbeatState, sequence: int) -> bytes:
    yaw = (state.yaw + sequence * 25) % 36000
    current_waypoint = 1 + ((state.current_waypoint + sequence // 10) % 6)
    battery_percent = max(0, state.battery_percent - sequence // 120)
    origin_distance = min(65535, state.origin_distance + sequence)
    target_distance = max(0, state.target_distance - sequence * 2)
    flight_time = min(65535, sequence)
    flight_range = min(65535, 200 + sequence * 5)
    utc_timestamp = int(time.time())

    payload = struct.pack(
        HEARTBEAT_PAYLOAD_FORMAT,
        state.aircraft_id,
        state.run_mode,
        state.satellite_count,
        state.longitude,
        state.latitude,
        state.altitude,
        state.relative_alt,
        state.airspeed,
        state.groundspeed,
        state.velocity_x,
        state.velocity_y,
        state.velocity_z,
        state.accel_x,
        state.accel_y,
        state.accel_z,
        state.roll,
        state.pitch,
        yaw,
        state.flight_mode,
        current_waypoint,
        state.battery_voltage,
        battery_percent,
        state.battery_current,
        state.atm_pressure,
        utc_timestamp,
        state.fault_info,
        state.rotation_speed,
        state.throttle,
        state.target_altitude,
        state.target_speed,
        origin_distance,
        state.origin_heading,
        target_distance,
        state.flight_state,
        state.altitude_state,
        state.state_switch_src,
        flight_time,
        flight_range,
        state.reserved,
    )

    if len(payload) != HEARTBEAT_PAYLOAD_LEN:
        raise RuntimeError(f"invalid heartbeat payload length: {len(payload)}")
    return payload


def build_heartbeat_frame(sequence: int, state: HeartbeatState) -> bytes:
    frame = build_frame(sequence, FRAME_TYPE_HEARTBEAT, build_heartbeat_payload(state, sequence))
    if len(frame) != HEARTBEAT_FRAME_LEN:
        raise RuntimeError(f"invalid heartbeat frame length: {len(frame)}")
    return frame


def get_expected_frame_len(frame_type: int, cmd_byte: int) -> int:
    if frame_type == FRAME_TYPE_HEARTBEAT:
        return HEARTBEAT_FRAME_LEN
    if frame_type == FRAME_TYPE_SET_DESTINATION:
        return 18
    if frame_type == FRAME_TYPE_REPLY:
        return 9
    if frame_type == FRAME_TYPE_GIMBAL_CONTROL:
        return 12
    if frame_type != FRAME_TYPE_COMMAND:
        return 0

    cmd_overhead = 8
    command_lengths = {
        CMD_SET_ROUTE: 0,
        CMD_SET_GEOFENCE: 0,
        CMD_SET_ANGLE: cmd_overhead + 4,
        CMD_SET_SPEED: cmd_overhead + 2,
        CMD_SET_ALTITUDE: cmd_overhead + 3,
        CMD_POWER_SWITCH: cmd_overhead + 1,
        CMD_PARACHUTE: cmd_overhead + 1,
        CMD_BUTTON: cmd_overhead + 1,
        CMD_SET_ORIGIN_RETURN: cmd_overhead + 11,
        CMD_SWITCH_MODE: cmd_overhead + 1,
        CMD_GUIDANCE: cmd_overhead + 19,
        CMD_GUIDANCE_NEW: cmd_overhead + 49,
        CMD_GIMBAL_ANG_RATE: cmd_overhead + 25,
        CMD_GIMBAL_ANGLE: cmd_overhead + 16,
        CMD_TARGET_STATE: cmd_overhead + 11,
    }
    return command_lengths.get(cmd_byte, 0)


def calc_variable_len_frame_len(buffer: bytearray, frame_type: int) -> int:
    if frame_type != FRAME_TYPE_COMMAND or len(buffer) < 6:
        return 0

    cmd_byte = buffer[4]
    point_count = buffer[5]
    var_overhead = 9

    if cmd_byte == CMD_SET_ROUTE:
        return var_overhead + point_count * 14
    if cmd_byte == CMD_SET_GEOFENCE:
        return var_overhead + point_count * 11
    return 0


def pop_frame(buffer: bytearray) -> ParsedFrame | None:
    while True:
        header_index = buffer.find(FRAME_HEADER)
        if header_index < 0:
            if len(buffer) > 1:
                del buffer[:-1]
            return None

        if header_index > 0:
            dirty = bytes(buffer[:header_index])
            del buffer[:header_index]
            log(f"丢弃串口脏数据: bytes={len(dirty)}, hex={format_hex(dirty)}")

        if len(buffer) < 5:
            return None

        frame_type = buffer[3]
        cmd_byte = buffer[4] if frame_type == FRAME_TYPE_COMMAND and len(buffer) >= 5 else 0
        expected_len = get_expected_frame_len(frame_type, cmd_byte)
        if expected_len == 0:
            expected_len = calc_variable_len_frame_len(buffer, frame_type)
            if expected_len == 0:
                if frame_type != FRAME_TYPE_COMMAND:
                    dropped = buffer.pop(0)
                    log(f"未知帧类型，继续同步: frame_type=0x{frame_type:02X}, dropped=0x{dropped:02X}")
                    continue
                return None

        if len(buffer) < expected_len:
            return None

        if bytes(buffer[expected_len - 2:expected_len]) != FRAME_TAIL:
            dropped = buffer.pop(0)
            log(f"帧尾不匹配，丢弃 1 字节继续同步: dropped=0x{dropped:02X}")
            continue

        checksum_index = expected_len - 3
        raw = bytes(buffer[:expected_len])
        received_checksum = buffer[checksum_index]
        calculated_checksum = calc_checksum(raw[:checksum_index])
        payload = bytes(buffer[4:checksum_index])

        del buffer[:expected_len]
        return ParsedFrame(
            cnt=raw[2],
            frame_type=raw[3],
            payload=payload,
            checksum=received_checksum,
            valid=calculated_checksum == received_checksum,
            raw=raw,
        )


def frame_type_name(frame_type: int) -> str:
    names = {
        FRAME_TYPE_HEARTBEAT: "飞控心跳",
        FRAME_TYPE_SET_DESTINATION: "设置目的地",
        FRAME_TYPE_COMMAND: "飞行控制指令",
        FRAME_TYPE_REPLY: "指令回复",
        FRAME_TYPE_GIMBAL_CONTROL: "云台控制",
    }
    return names.get(frame_type, "未知帧类型")


def command_name(cmd_type: int) -> str:
    names = {
        CMD_SET_DESTINATION: "设置飞行目的地",
        CMD_SET_ROUTE: "设置飞行航线",
        CMD_SET_ANGLE: "角度控制",
        CMD_SET_SPEED: "速度控制",
        CMD_SET_ALTITUDE: "高度控制",
        CMD_POWER_SWITCH: "载荷电源开关",
        CMD_PARACHUTE: "开伞控制",
        CMD_BUTTON: "指令按钮",
        CMD_SET_ORIGIN_RETURN: "设置原点/返航点",
        CMD_SET_GEOFENCE: "设置电子围栏",
        CMD_SWITCH_MODE: "切换运行模式",
        CMD_GUIDANCE: "末制导指令",
        CMD_GUIDANCE_NEW: "末制导指令(新版)",
        CMD_GIMBAL_ANG_RATE: "吊舱姿态-角速度模式",
        CMD_GIMBAL_ANGLE: "吊舱姿态-角度模式",
        CMD_TARGET_STATE: "识别目标状态",
    }
    return names.get(cmd_type, "未知指令")


def extract_reply_command(frame: ParsedFrame) -> int:
    if frame.payload:
        return frame.payload[0]
    return frame.frame_type


def safe_unlink(path: Path) -> None:
    if not path.exists() and not path.is_symlink():
        return
    if path.is_symlink() or str(path).startswith("/tmp/"):
        path.unlink(missing_ok=True)


def wait_for_path(path: Path, timeout_seconds: float = 5.0) -> None:
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        if path.exists():
            return
        time.sleep(0.05)
    raise TimeoutError(f"path not ready: {path}")


def describe_path(path: Path) -> str:
    try:
        target = path.resolve(strict=True)
        info = os.stat(target)
        mode = stat.S_IMODE(info.st_mode)
        owner = pwd.getpwuid(info.st_uid).pw_name
        group = grp.getgrgid(info.st_gid).gr_name
        return f"{path} -> {target}, mode={mode:03o}, owner={owner}, group={group}"
    except Exception as exc:
        return f"{path} -> <inspect failed: {exc}>"


def start_socat(app_port: Path, sim_port: Path) -> subprocess.Popen[bytes]:
    if shutil.which("socat") is None:
        raise RuntimeError("未找到 socat，请先安装 socat")

    safe_unlink(app_port)
    safe_unlink(sim_port)

    command = [
        "socat",
        "-d",
        "-d",
        f"PTY,link={app_port},mode=666,raw,echo=0",
        f"PTY,link={sim_port},mode=666,raw,echo=0",
    ]
    log("启动虚拟串口对: " + " ".join(command))
    process = subprocess.Popen(command, start_new_session=True)
    wait_for_path(app_port)
    wait_for_path(sim_port)
    log("虚拟串口权限快照: " + describe_path(app_port))
    log("虚拟串口权限快照: " + describe_path(sim_port))
    return process


def stop_socat(process: subprocess.Popen[bytes] | None) -> None:
    if process is None or process.poll() is not None:
        return

    try:
        os.killpg(process.pid, signal.SIGTERM)
        process.wait(timeout=2)
    except Exception:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except Exception:
            pass


def open_port(path: Path) -> int:
    return os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)


def maybe_send_reply(fd: int, frame: ParsedFrame, auto_reply: bool) -> None:
    if not auto_reply or not frame.valid:
        return
    if frame.frame_type not in (FRAME_TYPE_SET_DESTINATION, FRAME_TYPE_COMMAND):
        return

    replied_cmd = extract_reply_command(frame)
    reply = build_frame(frame.cnt, FRAME_TYPE_REPLY, bytes((replied_cmd, 0x00)))
    os.write(fd, reply)
    log(
        "已回程序侧指令回复: "
        f"cnt={frame.cnt}, reply_cmd=0x{replied_cmd:02X}({command_name(replied_cmd)}), "
        f"bytes={len(reply)}, hex={format_hex(reply)}"
    )


def run(args: argparse.Namespace) -> int:
    app_port = Path(args.app_port)
    sim_port = Path(args.sim_port)
    socat_process = None
    fd = None
    rx_buffer = bytearray()
    heartbeat_state = HeartbeatState()
    heartbeat_sequence = 0

    try:
        socat_process = start_socat(app_port, sim_port)
        fd = open_port(sim_port)

        log(f"虚拟串口已就绪: 程序侧端口={app_port}, 模拟器侧端口={sim_port}")
        log(f"当前脚本会向 {sim_port} 写入心跳，{app_port} 会收到数据")
        log(f"你的程序 fly_control 建议配置为 device={app_port}")
        log("模拟器功能: 周期发送心跳、打印程序下发指令、自动回成功回复")
        log("按 Ctrl+C 退出模拟器")

        next_heartbeat_time = time.monotonic()
        while True:
            if socat_process.poll() is not None:
                raise RuntimeError("socat 已退出，虚拟串口对失效")

            timeout = max(0.0, next_heartbeat_time - time.monotonic())
            readable, _, _ = select.select([fd], [], [], timeout)

            if readable:
                try:
                    chunk = os.read(fd, 4096)
                except BlockingIOError:
                    chunk = b""

                if chunk:
                    log(f"收到程序发来的串口数据: bytes={len(chunk)}, hex={format_hex(chunk)}")
                    rx_buffer.extend(chunk)
                    while True:
                        frame = pop_frame(rx_buffer)
                        if frame is None:
                            break

                        command_desc = ""
                        if frame.frame_type in (FRAME_TYPE_SET_DESTINATION, FRAME_TYPE_COMMAND) and frame.payload:
                            cmd_type = frame.payload[0]
                            command_desc = f", cmd=0x{cmd_type:02X}({command_name(cmd_type)})"

                        log(
                            "解析到程序侧完整帧: "
                            f"cnt={frame.cnt}, frame_type=0x{frame.frame_type:02X}({frame_type_name(frame.frame_type)}), "
                            f"payload_len={len(frame.payload)}, checksum=0x{frame.checksum:02X}, valid={frame.valid}{command_desc}, "
                            f"raw={format_hex(frame.raw)}"
                        )
                        maybe_send_reply(fd, frame, args.auto_reply)

            now = time.monotonic()
            if now >= next_heartbeat_time:
                frame = build_heartbeat_frame(heartbeat_sequence & 0xFF, heartbeat_state)
                os.write(fd, frame)
                log(
                    "已发送飞控心跳: "
                    f"seq={heartbeat_sequence}, cnt={heartbeat_sequence & 0xFF}, bytes={len(frame)}, hex={format_hex(frame)}"
                )
                heartbeat_sequence += 1
                next_heartbeat_time = now + args.interval

    except KeyboardInterrupt:
        log("收到 Ctrl+C，准备退出模拟器")
        return 0
    finally:
        if fd is not None:
            os.close(fd)
        stop_socat(socat_process)

    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="飞控串口模拟器：创建虚拟串口对，周期发送心跳，并打印程序发来的串口帧。"
    )
    parser.add_argument(
        "--app-port",
        default="/tmp/ttyV3_SIM",
        help="给 fast_cpp_server 使用的虚拟串口路径，默认与当前 config/config.json 一致",
    )
    parser.add_argument(
        "--sim-port",
        default="/tmp/ttyV3_FC",
        help="模拟器自己占用的虚拟串口路径",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=0.5,
        help="心跳发送周期，单位秒，默认 0.5",
    )
    parser.add_argument(
        "--no-auto-reply",
        dest="auto_reply",
        action="store_false",
        help="关闭对程序侧控制指令的自动成功回复",
    )
    parser.set_defaults(auto_reply=True)
    return parser.parse_args()


if __name__ == "__main__":
    sys.exit(run(parse_args()))