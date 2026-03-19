/**
 * @file base_image_capability.cpp
 * @brief 基础图像抓拍能力实现
 */

#include "base_image_capability.h"
#include "MyLog.h"

namespace PodModule {

BaseImageCapability::BaseImageCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseImageCapability 构造");
}

BaseImageCapability::~BaseImageCapability() {
    MYLOG_DEBUG("[吊舱能力] BaseImageCapability 析构");
}

CapabilityType BaseImageCapability::getType() const {
    return CapabilityType::IMAGE;
}

std::string BaseImageCapability::getName() const {
    return "图像抓拍";
}

PodResult<void> BaseImageCapability::initialize() {
    MYLOG_INFO("{} 初始化图像抓拍能力", logPrefix());
    return BaseCapability::initialize();
}

PodResult<ImageFrame> BaseImageCapability::captureImage() {
    MYLOG_INFO("{} 抓拍图像", logPrefix());
    // 基础占位：返回模拟图像帧
    ImageFrame frame;
    frame.width = 1920;
    frame.height = 1080;
    frame.channels = 3;
    frame.format = "JPEG";
    frame.timestamp_ms = 0; // TODO: 实际时间戳
    return PodResult<ImageFrame>::success(frame, "抓拍成功（基础占位）");
}

PodResult<std::string> BaseImageCapability::saveImage(const std::string& path) {
    MYLOG_INFO("{} 保存图像到: {}", logPrefix(), path);
    // 基础占位：不实际保存
    return PodResult<std::string>::success(path, "保存成功（基础占位）");
}

} // namespace PodModule
