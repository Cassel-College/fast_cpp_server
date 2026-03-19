/**
 * @file base_stream_capability.cpp
 * @brief 基础流媒体能力实现
 */

#include "base_stream_capability.h"
#include "MyLog.h"

namespace PodModule {

BaseStreamCapability::BaseStreamCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseStreamCapability 构造");
}

BaseStreamCapability::~BaseStreamCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseStreamCapability 析构");
    if (streaming_) {
        stopStream();
    }
}

CapabilityType BaseStreamCapability::getType() const {
    return CapabilityType::STREAM;
}

std::string BaseStreamCapability::getName() const {
    return "流媒体";
}

PodResult<void> BaseStreamCapability::initialize() {
    MYLOG_INFO("{} 初始化流媒体能力", logPrefix());
    return BaseCapability::initialize();
}

PodResult<StreamInfo> BaseStreamCapability::startStream(StreamType type) {
    MYLOG_INFO("{} 启动拉流，类型: {}", logPrefix(), streamTypeToString(type));
    if (streaming_) {
        MYLOG_WARN("{} 已在拉流中", logPrefix());
        return PodResult<StreamInfo>::success(current_stream_, "已在拉流中");
    }
    // 基础占位：设置模拟流信息
    current_stream_.type = type;
    current_stream_.url = "rtsp://127.0.0.1:8554/mock";
    current_stream_.width = 1920;
    current_stream_.height = 1080;
    current_stream_.fps = 30;
    current_stream_.is_active = true;
    streaming_ = true;
    return PodResult<StreamInfo>::success(current_stream_, "拉流启动成功（基础占位）");
}

PodResult<void> BaseStreamCapability::stopStream() {
    MYLOG_INFO("{} 停止拉流", logPrefix());
    if (!streaming_) {
        MYLOG_WARN("{} 当前未在拉流", logPrefix());
        return PodResult<void>::fail(PodErrorCode::STREAM_STOP_FAILED, "当前未在拉流");
    }
    streaming_ = false;
    current_stream_.is_active = false;
    return PodResult<void>::success("拉流已停止（基础占位）");
}

PodResult<StreamInfo> BaseStreamCapability::getStreamInfo() {
    MYLOG_DEBUG("{} 获取流信息", logPrefix());
    return PodResult<StreamInfo>::success(current_stream_, "获取流信息成功");
}

bool BaseStreamCapability::isStreaming() const {
    return streaming_;
}

} // namespace PodModule
