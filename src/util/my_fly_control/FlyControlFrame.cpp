#include "FlyControlFrame.h"
#include "MyLog.h"

#include <algorithm>
#include <cstring>

namespace fly_control {

// =============================================================================
// 校验和计算：buf[0..len-1] 所有字节求和，取低 8 位
// =============================================================================
uint8_t CalcChecksum(const uint8_t* data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum += data[i];
    }
    return static_cast<uint8_t>(sum & 0xFF);
}

// =============================================================================
// 组装完整帧
// 格式：帧头(2) + CNT(1) + 帧类型(1) + payload(N) + 校验和(1) + 帧尾(2)
// 校验和 = [帧头0 .. payload最后一字节] 所有字节的和取低8位
// =============================================================================
std::vector<uint8_t> BuildFrame(uint8_t cnt, uint8_t frame_type,
                                const std::vector<uint8_t>& payload) {
    // 总长度 = 帧头(2) + CNT(1) + 帧类型(1) + payload + 校验和(1) + 帧尾(2)
    size_t total = FRAME_HEADER_LEN + 1 + 1 + payload.size() +
                   FRAME_CHECKSUM_LEN + FRAME_TAIL_LEN;
    std::vector<uint8_t> frame;
    frame.reserve(total);

    // 帧头
    frame.push_back(FRAME_HEADER_0);
    frame.push_back(FRAME_HEADER_1);

    // CNT
    frame.push_back(cnt);

    // 帧类型
    frame.push_back(frame_type);

    // 载荷
    frame.insert(frame.end(), payload.begin(), payload.end());

    // 校验和：对当前所有字节求和
    uint8_t chk = CalcChecksum(frame.data(), frame.size());
    frame.push_back(chk);

    // 帧尾
    frame.push_back(FRAME_TAIL_0);
    frame.push_back(FRAME_TAIL_1);

    return frame;
}

// =============================================================================
// FrameParser 实现
// =============================================================================

void FrameParser::FeedData(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        buffer_.push_back(data[i]);
    }
}

void FrameParser::FeedData(const std::vector<uint8_t>& data) {
    FeedData(data.data(), data.size());
}

void FrameParser::Reset() {
    buffer_.clear();
    MYLOG_WARN("帧解析器已重置，缓冲区已清空");
}

size_t FrameParser::BufferSize() const {
    return buffer_.size();
}

// ---------------------------------------------------------------------------
// 在缓冲区中搜索帧头 0xEB 0x90，丢弃帧头之前的脏数据
// ---------------------------------------------------------------------------
bool FrameParser::FindHeader() {
    while (buffer_.size() >= 2) {
        if (buffer_[0] == FRAME_HEADER_0 && buffer_[1] == FRAME_HEADER_1) {
            return true;
        }
        // 丢弃一个无效字节，继续搜索
        buffer_.pop_front();
    }
    return false;
}

// ---------------------------------------------------------------------------
// 根据帧类型确定帧总长度
// ---------------------------------------------------------------------------
size_t FrameParser::GetExpectedFrameLen(uint8_t frame_type, uint8_t cmd_byte) const {
    switch (frame_type) {
        case FRAME_TYPE_HEARTBEAT:
            // 心跳帧固定 89 字节
            return HEARTBEAT_FRAME_LEN;

        case FRAME_TYPE_SET_DESTINATION:
            // 帧头(2)+CNT(1)+帧类型(1)+指令(1)+lon(4)+lat(4)+alt(2)+校验(1)+帧尾(2) = 18
            return 18;

        case FRAME_TYPE_REPLY:
            // 帧头(2)+CNT(1)+帧类型(1)+回复指令(1)+结果(1)+校验(1)+帧尾(2) = 9
            return 9;

        case FRAME_TYPE_GIMBAL_CONTROL:
            // 帧头(2)+CNT(1)+帧类型(1)+使能(1)+俯仰(2)+偏航(2)+校验(1)+帧尾(2) = 12
            return 12;

        case FRAME_TYPE_COMMAND: {
            // 指令帧需要根据子指令类型确定长度
            // 固定部分 = 帧头(2)+CNT(1)+帧类型(1)+指令类型(1) = 5
            // 再加上指令体+校验(1)+帧尾(2) = +3
            constexpr size_t CMD_OVERHEAD = 5 + 3; // 帧头CNT帧类型+指令类型 + 校验帧尾
            switch (cmd_byte) {
                case CMD_SET_ROUTE:    return 0;  // 变长，需要动态计算
                case CMD_SET_GEOFENCE: return 0;  // 变长，需要动态计算
                case CMD_SET_ANGLE:    return CMD_OVERHEAD + 4;  // pitch(2)+yaw(2)
                case CMD_SET_SPEED:    return CMD_OVERHEAD + 2;  // speed(2)
                case CMD_SET_ALTITUDE: return CMD_OVERHEAD + 3;  // type(1)+alt(2)
                case CMD_POWER_SWITCH: return CMD_OVERHEAD + 1;  // cmd(1)
                case CMD_PARACHUTE:    return CMD_OVERHEAD + 1;  // type(1)
                case CMD_BUTTON:       return CMD_OVERHEAD + 1;  // button(1)
                case CMD_SET_ORIGIN_RETURN: return CMD_OVERHEAD + 11; // type(1)+lon(4)+lat(4)+alt(2)
                case CMD_SWITCH_MODE:  return CMD_OVERHEAD + 1;  // mode(1)
                case CMD_GUIDANCE:     return CMD_OVERHEAD + 19; // mode(1)+frameid(4)+trackid(4)+lon(4)+lat(4)+alt(2)
                case CMD_GUIDANCE_NEW: return CMD_OVERHEAD + 49; // 很多字段
                case CMD_GIMBAL_ANG_RATE: return CMD_OVERHEAD + 25; // 6×4+1
                case CMD_GIMBAL_ANGLE: return CMD_OVERHEAD + 16; // 4×4
                case CMD_TARGET_STATE: return CMD_OVERHEAD + 11; // 2×4+3
                default:
                    return 0;  // 未知指令，无法确定长度
            }
        }

        default:
            return 0;  // 未知帧类型
    }
}

// ---------------------------------------------------------------------------
// 计算变长帧长度（航线帧和电子围栏帧）
// ---------------------------------------------------------------------------
size_t FrameParser::CalcVariableLenFrameLen(uint8_t frame_type) const {
    if (frame_type != FRAME_TYPE_COMMAND) {
        return 0;
    }

    // 至少需要 6 字节才能读到 指令类型(byte4) 和 点数量(byte5)
    if (buffer_.size() < 6) {
        return 0;
    }

    uint8_t cmd_byte = buffer_[4];
    uint8_t count    = buffer_[5];

    // 帧固定部分 = 帧头(2)+CNT(1)+帧类型(1)+指令类型(1)+点数量(1)+校验(1)+帧尾(2) = 9
    constexpr size_t VAR_OVERHEAD = 9;

    if (cmd_byte == CMD_SET_ROUTE) {
        // 每个航线点: lon(4)+lat(4)+alt(2)+index(2)+speed(2) = 14 字节
        return VAR_OVERHEAD + static_cast<size_t>(count) * 14;
    }

    if (cmd_byte == CMD_SET_GEOFENCE) {
        // 每个围栏点: lon(4)+lat(4)+alt(2)+index(1) = 11 字节
        return VAR_OVERHEAD + static_cast<size_t>(count) * 11;
    }

    return 0;
}

// ---------------------------------------------------------------------------
// 尝试从缓冲区中解析出一个完整帧
// ---------------------------------------------------------------------------
bool FrameParser::PopFrame(ParsedFrame& frame) {
    while (true) {
        // 步骤1：查找帧头
        if (!FindHeader()) {
            return false;
        }

        // 步骤2：至少需要 5 字节才能读到帧类型和可能的指令类型
        // 帧头(2) + CNT(1) + 帧类型(1) + 至少1字节载荷或校验
        if (buffer_.size() < 5) {
            return false;
        }

        uint8_t frame_type = buffer_[3];
        uint8_t cmd_byte   = 0;

        // 对于指令帧，第5字节是指令类型
        if (frame_type == FRAME_TYPE_COMMAND && buffer_.size() >= 5) {
            cmd_byte = buffer_[4];
        }

        // 步骤3：确定帧总长度
        size_t expected_len = GetExpectedFrameLen(frame_type, cmd_byte);

        if (expected_len == 0) {
            // 变长帧或未知帧类型，尝试动态计算
            expected_len = CalcVariableLenFrameLen(frame_type);
            if (expected_len == 0) {
                // 数据不够或未知帧类型
                if (frame_type != FRAME_TYPE_COMMAND) {
                    // 未知帧类型，丢弃帧头继续搜索
                    buffer_.pop_front();
                    continue;
                }
                // 指令帧但数据不够，等待更多数据
                return false;
            }
        }

        // 步骤4：检查缓冲区长度是否足够
        if (buffer_.size() < expected_len) {
            return false;  // 数据不够，等待更多数据
        }

        // 步骤5：校验帧尾
        if (buffer_[expected_len - 2] != FRAME_TAIL_0 ||
            buffer_[expected_len - 1] != FRAME_TAIL_1) {
            // 帧尾不匹配，当前帧头无效，丢弃继续搜索
            buffer_.pop_front();
            continue;
        }

        // 步骤6：校验和验证
        // 校验和位置 = expected_len - 3（帧尾2字节之前1字节）
        size_t chk_pos = expected_len - 3;
        uint8_t received_chk = buffer_[chk_pos];

        // 将缓冲区前 chk_pos 字节复制出来计算校验和
        std::vector<uint8_t> raw(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(chk_pos));
        uint8_t calc_chk = CalcChecksum(raw.data(), raw.size());

        // 步骤7：组装 ParsedFrame
        frame.cnt        = buffer_[2];
        frame.frame_type = buffer_[3];
        frame.checksum   = received_chk;
        frame.valid      = (calc_chk == received_chk);

        // 载荷 = 帧头(2)+CNT(1)+帧类型(1) 之后，到校验和之前
        size_t payload_start = 4;
        size_t payload_len   = chk_pos - payload_start;
        frame.payload.assign(buffer_.begin() + static_cast<std::ptrdiff_t>(payload_start),
                             buffer_.begin() + static_cast<std::ptrdiff_t>(payload_start + payload_len));

        // 步骤8：从缓冲区中移除已解析的帧
        for (size_t i = 0; i < expected_len; ++i) {
            buffer_.pop_front();
        }

        return true;
    }
}

} // namespace fly_control
