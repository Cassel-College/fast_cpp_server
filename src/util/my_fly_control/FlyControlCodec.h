#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "FlyControlProtocol.h"

// =============================================================================
// 协议编解码层
//
// 负责将协议数据结构与原始字节互相转换：
//   - Encode*  将结构体编码为完整帧（可直接通过串口发送）
//   - Decode*  从 ParsedFrame 的 payload 中解码出结构体
//
// 所有多字节字段均使用小端编码（MyLEHelper）
// =============================================================================

namespace fly_control {

// ===========================================================================
// 解码函数（帧载荷 → 数据结构）
// ===========================================================================

// 解码心跳帧载荷→HeartbeatData
bool DecodeHeartbeat(const std::vector<uint8_t>& payload, HeartbeatData& out);

// 解码云台控制帧载荷→GimbalControlData
bool DecodeGimbalControl(const std::vector<uint8_t>& payload, GimbalControlData& out);

// 解码指令回复帧载荷→CommandReplyData
bool DecodeCommandReply(const std::vector<uint8_t>& payload, CommandReplyData& out);

// ===========================================================================
// 编码函数（数据结构 → 完整帧字节序列）
// ===========================================================================

// 编码设置飞行目的地帧
std::vector<uint8_t> EncodeSetDestination(uint8_t cnt, const SetDestinationData& data);

// 编码设置飞行航线帧
std::vector<uint8_t> EncodeSetRoute(uint8_t cnt, const SetRouteData& data);

// 编码设置角度帧
std::vector<uint8_t> EncodeSetAngle(uint8_t cnt, const SetAngleData& data);

// 编码设置速度帧
std::vector<uint8_t> EncodeSetSpeed(uint8_t cnt, const SetSpeedData& data);

// 编码设置高度帧
std::vector<uint8_t> EncodeSetAltitude(uint8_t cnt, const SetAltitudeData& data);

// 编码电源开关帧
std::vector<uint8_t> EncodePowerSwitch(uint8_t cnt, const PowerSwitchData& data);

// 编码开伞控制帧
std::vector<uint8_t> EncodeParachute(uint8_t cnt, const ParachuteData& data);

// 编码指令按钮帧
std::vector<uint8_t> EncodeButtonCommand(uint8_t cnt, const ButtonCommandData& data);

// 编码设置原点/返航点帧
std::vector<uint8_t> EncodeSetOriginReturn(uint8_t cnt, const SetOriginReturnData& data);

// 编码设置电子围栏帧
std::vector<uint8_t> EncodeSetGeofence(uint8_t cnt, const SetGeofenceData& data);

// 编码切换运行模式帧
std::vector<uint8_t> EncodeSwitchMode(uint8_t cnt, const SwitchModeData& data);

// 编码末制导指令帧
std::vector<uint8_t> EncodeGuidance(uint8_t cnt, const GuidanceData& data);

// 编码末制导指令新版帧
std::vector<uint8_t> EncodeGuidanceNew(uint8_t cnt, const GuidanceNewData& data);

// 编码吊舱姿态-角速度模式帧
std::vector<uint8_t> EncodeGimbalAngRate(uint8_t cnt, const GimbalAngRateData& data);

// 编码吊舱姿态-角度模式帧
std::vector<uint8_t> EncodeGimbalAngle(uint8_t cnt, const GimbalAngleData& data);

// 编码识别目标状态帧
std::vector<uint8_t> EncodeTargetState(uint8_t cnt, const TargetStateData& data);

// ===========================================================================
// 辅助函数
// ===========================================================================

// 获取指令类型的中文名称（用于日志）
std::string GetCommandName(uint8_t cmd_type);

// 获取帧类型的中文名称（用于日志）
std::string GetFrameTypeName(uint8_t frame_type);

} // namespace fly_control
