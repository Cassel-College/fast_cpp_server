/**
 * @file dji_stream_capability.cpp
 * @brief 大疆流媒体能力实现
 */

#include "dji_stream_capability.h"
#include "MyLog.h"

namespace PodModule {

DjiStreamCapability::DjiStreamCapability() {
    MYLOG_DEBUG("[大疆能力] DjiStreamCapability 构造");
}

DjiStreamCapability::~DjiStreamCapability() {
    MYLOG_DEBUG("[大疆能力] DjiStreamCapability 析构");
}

PodResult<StreamInfo> DjiStreamCapability::startStream(StreamType type) {
    MYLOG_INFO("{} 启动大疆拉流，类型: {}", logPrefix(), streamTypeToString(type));
    // TODO: 通过 Session 启动大疆视频流
    current_stream_.type = type;
    current_stream_.url = "rtsp://192.168.1.100:8554/dji_stream";
    current_stream_.width = 1920;
    current_stream_.height = 1080;
    current_stream_.fps = 30;
    current_stream_.is_active = true;
    streaming_ = true;
    return PodResult<StreamInfo>::success(current_stream_, "大疆拉流启动成功（占位）");
}

PodResult<void> DjiStreamCapability::stopStream() {
    MYLOG_INFO("{} 停止大疆拉流", logPrefix());
    // TODO: 通过 Session 停止大疆视频流
    streaming_ = false;
    current_stream_.is_active = false;
    return PodResult<void>::success("大疆拉流已停止（占位）");
}

} // namespace PodModule
