#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

#include "FlyControlProtocol.h"

// =============================================================================
// 帧层：负责帧同步、字节流拆帧、校验和验证、帧组装
//
// 功能：
//   1. BuildFrame    - 将载荷数据组装成完整帧（添加帧头、CNT、帧类型、校验和、帧尾）
//   2. CalcChecksum  - 计算校验和（所有字节求和取低8位）
//   3. FeedData      - 向帧解析器喂入字节流
//   4. PopFrame      - 从解析器中取出一个完整且校验通过的帧
//   5. Reset         - 重置帧解析器状态
// =============================================================================

namespace fly_control {

// ---------------------------------------------------------------------------
// 帧构建工具（静态方法）
// ---------------------------------------------------------------------------

// 计算校验和：buf 中 [0, len) 所有字节求和，取低8位
uint8_t CalcChecksum(const uint8_t* data, size_t len);

// 组装完整帧：帧头 + cnt + frame_type + payload + 校验和 + 帧尾
// 返回完整帧的字节序列
std::vector<uint8_t> BuildFrame(uint8_t cnt, uint8_t frame_type,
                                const std::vector<uint8_t>& payload);

// ---------------------------------------------------------------------------
// 帧解析器：从连续字节流中拆出完整帧
// ---------------------------------------------------------------------------
class FrameParser {
public:
    FrameParser() = default;

    // 喂入一段字节数据（可能包含多帧、半帧、粘包）
    void FeedData(const uint8_t* data, size_t len);
    void FeedData(const std::vector<uint8_t>& data);

    // 尝试取出一个完整帧，成功返回 true 并填充 frame
    bool PopFrame(ParsedFrame& frame);

    // 重置解析器内部状态
    void Reset();

    // 当前缓冲区中待处理字节数
    size_t BufferSize() const;

private:
    // 在缓冲区中查找帧头位置，丢弃帧头之前的无效字节
    // 返回是否找到帧头
    bool FindHeader();

    // 根据帧类型确定帧总长度（含帧头帧尾）
    // 对于变长帧返回 0 表示需要动态计算
    size_t GetExpectedFrameLen(uint8_t frame_type, uint8_t cmd_byte) const;

    // 尝试确定变长帧的总长度
    // 返回 0 表示数据不够无法确定
    size_t CalcVariableLenFrameLen(uint8_t frame_type) const;

    std::deque<uint8_t> buffer_;   // 接收缓冲区
};

} // namespace fly_control
