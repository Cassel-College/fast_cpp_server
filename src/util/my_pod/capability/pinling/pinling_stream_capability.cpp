/**
 * @file pinling_stream_capability.cpp
 * @brief 品凌流媒体能力实现
 */

#include "pinling_stream_capability.h"
#include "MyLog.h"

namespace PodModule {

PinlingStreamCapability::PinlingStreamCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingStreamCapability 构造");
}

PinlingStreamCapability::~PinlingStreamCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingStreamCapability 析构");
}

PodResult<StreamInfo> PinlingStreamCapability::startStream(StreamType type) {
    MYLOG_INFO("{} 启动品凌拉流，类型: {}", logPrefix(), streamTypeToString(type));
    // TODO: 通过 Session 启动品凌视频流
    current_stream_.type = type;
    current_stream_.url = "rtsp://192.168.1.200:554/pinling_stream";
    current_stream_.width = 1920;
    current_stream_.height = 1080;
    current_stream_.fps = 25;
    current_stream_.is_active = true;
    streaming_ = true;
    return PodResult<StreamInfo>::success(current_stream_, "品凌拉流启动成功（占位）");
}

PodResult<void> PinlingStreamCapability::stopStream() {
    MYLOG_INFO("{} 停止品凌拉流", logPrefix());
    streaming_ = false;
    current_stream_.is_active = false;
    return PodResult<void>::success("品凌拉流已停止（占位）");
}

} // namespace PodModule
