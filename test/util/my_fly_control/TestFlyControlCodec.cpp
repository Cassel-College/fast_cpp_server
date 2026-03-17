#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

#include "FlyControlCodec.h"
#include "FlyControlFrame.h"
#include "FlyControlProtocol.h"
#include "MyLEHelper.h"

using namespace fly_control;

// =============================================================================
// 协议编解码层单元测试
// =============================================================================

// ---- 心跳帧编解码回环测试 ----
TEST(FlyControlCodecTest, HeartbeatEncodeDecodeRoundTrip) {
    // 手动构建心跳帧载荷（模拟飞控发来的数据）
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, 0x01);     // 飞机ID
    MyLEHelper::write_uint8(payload, 0x00);     // 运行模式=实飞
    MyLEHelper::write_uint8(payload, 12);       // 定位星数=12
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(1163456789));  // lon
    MyLEHelper::write_uint32(payload, static_cast<uint32_t>(396789012));   // lat
    MyLEHelper::write_uint16(payload, 1500);    // alt = 150.0m
    MyLEHelper::write_uint16(payload, 500);     // relative_alt = 50.0m
    MyLEHelper::write_uint16(payload, 2500);    // airspeed = 25.00 m/s
    MyLEHelper::write_uint16(payload, 2300);    // groundspeed = 23.00 m/s
    MyLEHelper::write_int16(payload, 100);      // vx
    MyLEHelper::write_int16(payload, -50);      // vy
    MyLEHelper::write_int16(payload, 10);       // vz
    MyLEHelper::write_int16(payload, 5);        // ax
    MyLEHelper::write_int16(payload, -3);       // ay
    MyLEHelper::write_int16(payload, 2);        // az
    MyLEHelper::write_int16(payload, 150);      // roll = 1.50 deg
    MyLEHelper::write_int16(payload, -200);     // pitch = -2.00 deg
    MyLEHelper::write_uint16(payload, 18000);   // yaw = 180.00 deg
    MyLEHelper::write_uint8(payload, 0x03);     // flight_mode = 任务执行中
    MyLEHelper::write_uint16(payload, 5);       // current_waypoint = 5
    MyLEHelper::write_uint16(payload, 2200);    // battery_voltage = 22.00V
    MyLEHelper::write_uint8(payload, 75);       // battery_percent = 75%
    MyLEHelper::write_uint16(payload, 150);     // battery_current = 15.0A
    MyLEHelper::write_uint16(payload, 10130);   // atm_pressure = 101.30 kPa
    MyLEHelper::write_uint64(payload, 1710600000000ULL);  // UTC时间戳
    MyLEHelper::write_uint16(payload, 0x0041);  // fault_info (链路丢失+电压过低)
    MyLEHelper::write_uint16(payload, 3000);    // rotation_speed
    MyLEHelper::write_uint16(payload, 50);      // throttle
    MyLEHelper::write_uint16(payload, 1000);    // target_altitude = 100.0m
    MyLEHelper::write_uint16(payload, 2000);    // target_speed = 20.00 m/s
    MyLEHelper::write_uint16(payload, 500);     // origin_distance = 500m
    MyLEHelper::write_uint16(payload, 9000);    // origin_heading = 90.00 deg
    MyLEHelper::write_uint16(payload, 1200);    // target_distance = 1200m
    MyLEHelper::write_uint8(payload, 0x04);     // flight_state = 巡航
    MyLEHelper::write_uint8(payload, 0x00);     // altitude_state
    MyLEHelper::write_uint8(payload, 0x00);     // state_switch_src
    MyLEHelper::write_uint16(payload, 600);     // flight_time = 600s
    MyLEHelper::write_uint16(payload, 15000);   // flight_range = 15000m
    MyLEHelper::write_uint32(payload, 0x00);    // reserved

    EXPECT_EQ(payload.size(), 82u);

    // 解码
    HeartbeatData hb;
    EXPECT_TRUE(DecodeHeartbeat(payload, hb));

    // 验证关键字段
    EXPECT_EQ(hb.aircraft_id, 0x01);
    EXPECT_EQ(hb.run_mode, 0x00);
    EXPECT_EQ(hb.satellite_count, 12);
    EXPECT_EQ(hb.longitude, 1163456789);
    EXPECT_EQ(hb.latitude, 396789012);
    EXPECT_EQ(hb.altitude, 1500);
    EXPECT_EQ(hb.relative_alt, 500);
    EXPECT_EQ(hb.airspeed, 2500);
    EXPECT_EQ(hb.velocity_x, 100);
    EXPECT_EQ(hb.velocity_y, -50);
    EXPECT_EQ(hb.roll, 150);
    EXPECT_EQ(hb.pitch, -200);
    EXPECT_EQ(hb.yaw, 18000);
    EXPECT_EQ(hb.flight_mode, 0x03);
    EXPECT_EQ(hb.current_waypoint, 5);
    EXPECT_EQ(hb.battery_voltage, 2200);
    EXPECT_EQ(hb.battery_percent, 75);
    EXPECT_EQ(hb.utc_timestamp, 1710600000000ULL);
    EXPECT_EQ(hb.fault_info, 0x0041);
    EXPECT_EQ(hb.flight_state, 0x04);
    EXPECT_EQ(hb.flight_time, 600);
    EXPECT_EQ(hb.flight_range, 15000);

    // 验证故障位解析
    auto faults = FaultBits::FromU16(hb.fault_info);
    EXPECT_TRUE(faults.link_lost);
    EXPECT_TRUE(faults.voltage_too_low);
    EXPECT_FALSE(faults.gps_lost);
    EXPECT_FALSE(faults.parachute_failed);
}

// ---- FaultBits 回环测试 ----
TEST(FlyControlCodecTest, FaultBitsRoundTrip) {
    FaultBits fb;
    fb.link_lost = true;
    fb.gps_lost = true;
    fb.parachute_failed = true;
    fb.altitude_too_high = true;

    uint16_t val = fb.ToU16();
    auto fb2 = FaultBits::FromU16(val);

    EXPECT_TRUE(fb2.link_lost);
    EXPECT_TRUE(fb2.gps_lost);
    EXPECT_TRUE(fb2.parachute_failed);
    EXPECT_TRUE(fb2.altitude_too_high);
    EXPECT_FALSE(fb2.takeoff_failed);
    EXPECT_FALSE(fb2.voltage_too_low);
}

// ---- 设置目的地编解码 ----
TEST(FlyControlCodecTest, SetDestinationEncodeDecode) {
    SetDestinationData data;
    data.longitude = 1163456789;
    data.latitude  = 396789012;
    data.altitude  = 1500;

    auto frame = EncodeSetDestination(0x01, data);

    // 验证帧结构
    EXPECT_EQ(frame[0], FRAME_HEADER_0);
    EXPECT_EQ(frame[1], FRAME_HEADER_1);
    EXPECT_EQ(frame[2], 0x01);  // CNT
    EXPECT_EQ(frame[3], FRAME_TYPE_SET_DESTINATION);

    // 用帧解析器解出来
    FrameParser parser;
    parser.FeedData(frame);

    ParsedFrame pf;
    EXPECT_TRUE(parser.PopFrame(pf));
    EXPECT_TRUE(pf.valid);
    EXPECT_EQ(pf.frame_type, FRAME_TYPE_SET_DESTINATION);

    // 验证载荷内容
    size_t off = 0;
    uint8_t cmd = MyLEHelper::read_uint8(pf.payload, off);
    EXPECT_EQ(cmd, CMD_SET_DESTINATION);
    int32_t lon = static_cast<int32_t>(MyLEHelper::read_uint32(pf.payload, off));
    int32_t lat = static_cast<int32_t>(MyLEHelper::read_uint32(pf.payload, off));
    uint16_t alt = MyLEHelper::read_uint16(pf.payload, off);
    EXPECT_EQ(lon, 1163456789);
    EXPECT_EQ(lat, 396789012);
    EXPECT_EQ(alt, 1500);
}

// ---- 设置航线编解码（变长帧）----
TEST(FlyControlCodecTest, SetRouteEncodeDecodeVariableLength) {
    SetRouteData data;
    data.points.push_back({1163000000, 396000000, 1000, 1, 2000});
    data.points.push_back({1163100000, 396100000, 1500, 2, 2500});
    data.points.push_back({1163200000, 396200000, 2000, 3, 1500});

    auto frame = EncodeSetRoute(0x02, data);

    // 帧解析
    FrameParser parser;
    parser.FeedData(frame);

    ParsedFrame pf;
    EXPECT_TRUE(parser.PopFrame(pf));
    EXPECT_TRUE(pf.valid);
    EXPECT_EQ(pf.frame_type, FRAME_TYPE_COMMAND);

    // 验证载荷
    size_t off = 0;
    uint8_t cmd = MyLEHelper::read_uint8(pf.payload, off);
    EXPECT_EQ(cmd, CMD_SET_ROUTE);
    uint8_t count = MyLEHelper::read_uint8(pf.payload, off);
    EXPECT_EQ(count, 3);

    // 第一个航线点
    int32_t lon1 = static_cast<int32_t>(MyLEHelper::read_uint32(pf.payload, off));
    int32_t lat1 = static_cast<int32_t>(MyLEHelper::read_uint32(pf.payload, off));
    uint16_t alt1 = MyLEHelper::read_uint16(pf.payload, off);
    uint16_t idx1 = MyLEHelper::read_uint16(pf.payload, off);
    uint16_t spd1 = MyLEHelper::read_uint16(pf.payload, off);
    EXPECT_EQ(lon1, 1163000000);
    EXPECT_EQ(lat1, 396000000);
    EXPECT_EQ(alt1, 1000);
    EXPECT_EQ(idx1, 1);
    EXPECT_EQ(spd1, 2000);
}

// ---- 设置速度编码帧完整性 ----
TEST(FlyControlCodecTest, SetSpeedFrameIntegrity) {
    SetSpeedData data;
    data.speed = 1500;  // 15.00 m/s

    auto frame = EncodeSetSpeed(0x0A, data);

    // 帧头(2)+CNT(1)+帧类型(1)+指令(1)+速度(2)+校验(1)+帧尾(2) = 10
    EXPECT_EQ(frame.size(), 10u);

    // 通过帧解析器验证
    FrameParser parser;
    parser.FeedData(frame);
    ParsedFrame pf;
    EXPECT_TRUE(parser.PopFrame(pf));
    EXPECT_TRUE(pf.valid);

    size_t off = 0;
    EXPECT_EQ(MyLEHelper::read_uint8(pf.payload, off), CMD_SET_SPEED);
    EXPECT_EQ(MyLEHelper::read_uint16(pf.payload, off), 1500);
}

// ---- 设置高度编码 ----
TEST(FlyControlCodecTest, SetAltitudeEncode) {
    SetAltitudeData data;
    data.altitude_type = 0x01;  // 相对高度
    data.altitude = 500;       // 50.0m

    auto frame = EncodeSetAltitude(0x03, data);

    FrameParser parser;
    parser.FeedData(frame);
    ParsedFrame pf;
    EXPECT_TRUE(parser.PopFrame(pf));
    EXPECT_TRUE(pf.valid);

    size_t off = 0;
    EXPECT_EQ(MyLEHelper::read_uint8(pf.payload, off), CMD_SET_ALTITUDE);
    EXPECT_EQ(MyLEHelper::read_uint8(pf.payload, off), 0x01);
    EXPECT_EQ(MyLEHelper::read_uint16(pf.payload, off), 500);
}

// ---- 指令回复解码 ----
TEST(FlyControlCodecTest, CommandReplyDecode) {
    // 构建回复帧载荷
    std::vector<uint8_t> payload = {CMD_SET_SPEED, 0x00};  // 速度控制成功

    CommandReplyData reply;
    EXPECT_TRUE(DecodeCommandReply(payload, reply));
    EXPECT_EQ(reply.replied_cmd, CMD_SET_SPEED);
    EXPECT_EQ(reply.result, 0x00);
}

// ---- 云台控制解码 ----
TEST(FlyControlCodecTest, GimbalControlDecode) {
    std::vector<uint8_t> payload;
    MyLEHelper::write_uint8(payload, 1);       // 使能
    MyLEHelper::write_int16(payload, -1500);   // 俯仰 = -15.00 度
    MyLEHelper::write_int16(payload, 3000);    // 偏航 = 30.00 度

    GimbalControlData gc;
    EXPECT_TRUE(DecodeGimbalControl(payload, gc));
    EXPECT_EQ(gc.enable, 1);
    EXPECT_EQ(gc.pitch_angle, -1500);
    EXPECT_EQ(gc.yaw_angle, 3000);
}

// ---- 电子围栏变长帧 ----
TEST(FlyControlCodecTest, GeofenceEncodeDecodeVariableLength) {
    SetGeofenceData data;
    data.points.push_back({1163000000, 396000000, 1000, 0});
    data.points.push_back({1163100000, 396100000, 1000, 1});
    data.points.push_back({1163200000, 396200000, 1000, 2});
    data.points.push_back({1163000000, 396200000, 1000, 3});

    auto frame = EncodeSetGeofence(0x05, data);

    FrameParser parser;
    parser.FeedData(frame);
    ParsedFrame pf;
    EXPECT_TRUE(parser.PopFrame(pf));
    EXPECT_TRUE(pf.valid);

    size_t off = 0;
    EXPECT_EQ(MyLEHelper::read_uint8(pf.payload, off), CMD_SET_GEOFENCE);
    uint8_t count = MyLEHelper::read_uint8(pf.payload, off);
    EXPECT_EQ(count, 4);

    // 第一个围栏点
    int32_t lon = static_cast<int32_t>(MyLEHelper::read_uint32(pf.payload, off));
    EXPECT_EQ(lon, 1163000000);
}

// ---- 末制导指令编码 ----
TEST(FlyControlCodecTest, GuidanceEncode) {
    GuidanceData data;
    data.mode_switch = 1;
    data.frame_id    = 100;
    data.track_id    = 200;
    data.target_lon  = 1163456789;
    data.target_lat  = 396789012;
    data.target_alt  = 500;

    auto frame = EncodeGuidance(0x07, data);

    FrameParser parser;
    parser.FeedData(frame);
    ParsedFrame pf;
    EXPECT_TRUE(parser.PopFrame(pf));
    EXPECT_TRUE(pf.valid);

    size_t off = 0;
    EXPECT_EQ(MyLEHelper::read_uint8(pf.payload, off), CMD_GUIDANCE);
    EXPECT_EQ(MyLEHelper::read_uint8(pf.payload, off), 1);        // mode_switch
    EXPECT_EQ(MyLEHelper::read_uint32(pf.payload, off), 100u);    // frame_id
    EXPECT_EQ(MyLEHelper::read_uint32(pf.payload, off), 200u);    // track_id
}

// ---- 帧类型/指令名称获取 ----
TEST(FlyControlCodecTest, GetFrameAndCommandNames) {
    EXPECT_EQ(GetFrameTypeName(FRAME_TYPE_HEARTBEAT), "飞控心跳");
    EXPECT_EQ(GetFrameTypeName(FRAME_TYPE_REPLY), "指令回复");
    EXPECT_EQ(GetFrameTypeName(0xFF), "未知帧类型");

    EXPECT_EQ(GetCommandName(CMD_SET_SPEED), "速度控制");
    EXPECT_EQ(GetCommandName(CMD_SET_ROUTE), "设置飞行航线");
    EXPECT_EQ(GetCommandName(CMD_PARACHUTE), "开伞控制");
    EXPECT_EQ(GetCommandName(0xFF), "未知指令");
}

// ---- 帧编码后经解析器再解码的端到端测试 ----
TEST(FlyControlCodecTest, EndToEndHeartbeatFrameParsing) {
    // 构建完整心跳帧（模拟飞控发来的原始字节流）
    std::vector<uint8_t> payload(82, 0x00);
    payload[0] = 0x02;   // aircraft_id = 2
    payload[1] = 0x01;   // run_mode = 仿真
    payload[2] = 0x08;   // satellite_count = 8

    auto raw = BuildFrame(0x20, FRAME_TYPE_HEARTBEAT, payload);

    // 帧解析
    FrameParser parser;
    parser.FeedData(raw);

    ParsedFrame pf;
    EXPECT_TRUE(parser.PopFrame(pf));
    EXPECT_TRUE(pf.valid);
    EXPECT_EQ(pf.frame_type, FRAME_TYPE_HEARTBEAT);
    EXPECT_EQ(pf.cnt, 0x20);

    // 协议解码
    HeartbeatData hb;
    hb.cnt = pf.cnt;
    EXPECT_TRUE(DecodeHeartbeat(pf.payload, hb));
    EXPECT_EQ(hb.cnt, 0x20);
    EXPECT_EQ(hb.aircraft_id, 0x02);
    EXPECT_EQ(hb.run_mode, 0x01);
    EXPECT_EQ(hb.satellite_count, 8);
}

// ---- 原点/返航点编码 ----
TEST(FlyControlCodecTest, SetOriginReturnEncode) {
    SetOriginReturnData data;
    data.point_type = 0x01;  // 返航点
    data.longitude  = 1163000000;
    data.latitude   = 396000000;
    data.altitude   = 2000;  // 200.0m

    auto frame = EncodeSetOriginReturn(0x08, data);

    FrameParser parser;
    parser.FeedData(frame);
    ParsedFrame pf;
    EXPECT_TRUE(parser.PopFrame(pf));
    EXPECT_TRUE(pf.valid);

    size_t off = 0;
    EXPECT_EQ(MyLEHelper::read_uint8(pf.payload, off), CMD_SET_ORIGIN_RETURN);
    EXPECT_EQ(MyLEHelper::read_uint8(pf.payload, off), 0x01);
    int32_t lon = static_cast<int32_t>(MyLEHelper::read_uint32(pf.payload, off));
    EXPECT_EQ(lon, 1163000000);
}
