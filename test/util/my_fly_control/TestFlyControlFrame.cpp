#include "gtest/gtest.h"

#include <vector>
#include <cstdint>

#include "FlyControlFrame.h"
#include "FlyControlProtocol.h"

using namespace fly_control;

// =============================================================================
// 帧层单元测试
// =============================================================================

// ---- 校验和计算 ----
TEST(FlyControlFrameTest, CalcChecksumBasic) {
    // 验证几个字节的和校验
    uint8_t data[] = {0xEB, 0x90, 0x01, 0x10, 0x04, 0x00, 0x10};
    uint8_t chk = CalcChecksum(data, sizeof(data));
    // 0xEB + 0x90 + 0x01 + 0x10 + 0x04 + 0x00 + 0x10 = 0x1A0 → 低8位 = 0xA0
    EXPECT_EQ(chk, 0xA0);
}

// ---- BuildFrame 基本测试 ----
TEST(FlyControlFrameTest, BuildFrameStructure) {
    std::vector<uint8_t> payload = {0x04, 0xE8, 0x03};  // 指令类型+速度
    auto frame = BuildFrame(0x05, FRAME_TYPE_COMMAND, payload);

    // 验证帧头
    EXPECT_EQ(frame[0], FRAME_HEADER_0);
    EXPECT_EQ(frame[1], FRAME_HEADER_1);

    // 验证 CNT
    EXPECT_EQ(frame[2], 0x05);

    // 验证帧类型
    EXPECT_EQ(frame[3], FRAME_TYPE_COMMAND);

    // 验证载荷
    EXPECT_EQ(frame[4], 0x04);
    EXPECT_EQ(frame[5], 0xE8);
    EXPECT_EQ(frame[6], 0x03);

    // 验证帧尾（最后两个字节）
    EXPECT_EQ(frame[frame.size() - 2], FRAME_TAIL_0);
    EXPECT_EQ(frame[frame.size() - 1], FRAME_TAIL_1);

    // 验证校验和位置（倒数第3字节）
    size_t chk_pos = frame.size() - 3;
    uint8_t expected_chk = CalcChecksum(frame.data(), chk_pos);
    EXPECT_EQ(frame[chk_pos], expected_chk);
}

// ---- BuildFrame 空载荷 ----
TEST(FlyControlFrameTest, BuildFrameEmptyPayload) {
    std::vector<uint8_t> payload;
    auto frame = BuildFrame(0x00, 0xFF, payload);
    // 帧头(2) + CNT(1) + 帧类型(1) + 校验(1) + 帧尾(2) = 7
    EXPECT_EQ(frame.size(), 7u);
}

// ---- FrameParser 解析单帧 ----
TEST(FlyControlFrameTest, ParseSingleFrame) {
    // 构建一个指令帧
    std::vector<uint8_t> payload = {0x04, 0xD0, 0x07};
    auto raw = BuildFrame(0x01, FRAME_TYPE_COMMAND, payload);

    // 喂入解析器
    FrameParser parser;
    parser.FeedData(raw);

    ParsedFrame frame;
    EXPECT_TRUE(parser.PopFrame(frame));
    EXPECT_TRUE(frame.valid);
    EXPECT_EQ(frame.cnt, 0x01);
    EXPECT_EQ(frame.frame_type, FRAME_TYPE_COMMAND);
    EXPECT_EQ(frame.payload.size(), payload.size());
    EXPECT_EQ(frame.payload[0], 0x04);

    // 不应该再有更多帧
    EXPECT_FALSE(parser.PopFrame(frame));
}

// ---- FrameParser 多帧粘包 ----
TEST(FlyControlFrameTest, ParseMultipleFrames) {
    std::vector<uint8_t> p1 = {0x04, 0x10, 0x27};
    std::vector<uint8_t> p2 = {0x06, 0x01};
    auto f1 = BuildFrame(0x01, FRAME_TYPE_COMMAND, p1);
    auto f2 = BuildFrame(0x02, FRAME_TYPE_COMMAND, p2);

    // 拼接两帧
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), f1.begin(), f1.end());
    combined.insert(combined.end(), f2.begin(), f2.end());

    FrameParser parser;
    parser.FeedData(combined);

    ParsedFrame frame;
    EXPECT_TRUE(parser.PopFrame(frame));
    EXPECT_EQ(frame.cnt, 0x01);
    EXPECT_EQ(frame.payload[0], 0x04);

    EXPECT_TRUE(parser.PopFrame(frame));
    EXPECT_EQ(frame.cnt, 0x02);
    EXPECT_EQ(frame.payload[0], 0x06);

    EXPECT_FALSE(parser.PopFrame(frame));
}

// ---- FrameParser 含脏数据 ----
TEST(FlyControlFrameTest, ParseWithGarbageData) {
    std::vector<uint8_t> payload = {0x0C, 0x01};
    auto valid_frame = BuildFrame(0x03, FRAME_TYPE_COMMAND, payload);

    // 在有效帧前加入一些垃圾数据
    std::vector<uint8_t> data = {0xFF, 0xAA, 0x55, 0x00};
    data.insert(data.end(), valid_frame.begin(), valid_frame.end());

    FrameParser parser;
    parser.FeedData(data);

    ParsedFrame frame;
    EXPECT_TRUE(parser.PopFrame(frame));
    EXPECT_TRUE(frame.valid);
    EXPECT_EQ(frame.cnt, 0x03);
    EXPECT_EQ(frame.payload[0], 0x0C);
}

// ---- FrameParser 半帧（数据不够）----
TEST(FlyControlFrameTest, ParseIncompleteFrame) {
    std::vector<uint8_t> payload = {0x09, 0x01};
    auto full = BuildFrame(0x01, FRAME_TYPE_COMMAND, payload);

    // 只喂入一半数据
    std::vector<uint8_t> half(full.begin(), full.begin() + full.size() / 2);

    FrameParser parser;
    parser.FeedData(half);

    ParsedFrame frame;
    EXPECT_FALSE(parser.PopFrame(frame));

    // 喂入剩余数据
    std::vector<uint8_t> rest(full.begin() + full.size() / 2, full.end());
    parser.FeedData(rest);

    EXPECT_TRUE(parser.PopFrame(frame));
    EXPECT_TRUE(frame.valid);
    EXPECT_EQ(frame.cnt, 0x01);
}

// ---- FrameParser 解析心跳帧 ----
TEST(FlyControlFrameTest, ParseHeartbeatFrame) {
    // 构建 89 字节心跳帧
    // payload = 82 字节（在 BuildFrame 中 payload 不包含 帧头/CNT/帧类型）
    std::vector<uint8_t> payload(82, 0x00);
    payload[0] = 0x01;  // 飞机ID = 1
    payload[1] = 0x00;  // 运行模式 = 实飞
    payload[2] = 0x0A;  // 10颗星

    auto raw = BuildFrame(0x10, FRAME_TYPE_HEARTBEAT, payload);
    EXPECT_EQ(raw.size(), 89u);

    FrameParser parser;
    parser.FeedData(raw);

    ParsedFrame frame;
    EXPECT_TRUE(parser.PopFrame(frame));
    EXPECT_TRUE(frame.valid);
    EXPECT_EQ(frame.frame_type, FRAME_TYPE_HEARTBEAT);
    EXPECT_EQ(frame.payload.size(), 82u);
    EXPECT_EQ(frame.payload[0], 0x01);
    EXPECT_EQ(frame.payload[2], 0x0A);
}

// ---- FrameParser 解析回复帧 ----
TEST(FlyControlFrameTest, ParseReplyFrame) {
    // 回复帧: 回复指令类型(1) + 结果(1) = 2字节 payload
    std::vector<uint8_t> payload = {0x04, 0x00};  // 回复速度控制, 成功
    auto raw = BuildFrame(0x05, FRAME_TYPE_REPLY, payload);
    EXPECT_EQ(raw.size(), 9u);  // 2+1+1+2+1+2 = 9

    FrameParser parser;
    parser.FeedData(raw);

    ParsedFrame frame;
    EXPECT_TRUE(parser.PopFrame(frame));
    EXPECT_TRUE(frame.valid);
    EXPECT_EQ(frame.frame_type, FRAME_TYPE_REPLY);
    EXPECT_EQ(frame.payload[0], 0x04);
    EXPECT_EQ(frame.payload[1], 0x00);
}

// ---- FrameParser Reset ----
TEST(FlyControlFrameTest, ResetClearsBuffer) {
    FrameParser parser;
    std::vector<uint8_t> data = {0xEB, 0x90, 0x01, 0x10};
    parser.FeedData(data);
    EXPECT_GT(parser.BufferSize(), 0u);

    parser.Reset();
    EXPECT_EQ(parser.BufferSize(), 0u);
}
