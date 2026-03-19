/**
 * @file pinling_image_capability.cpp
 * @brief 品凌图像抓拍能力实现
 */

#include "pinling_image_capability.h"
#include "MyLog.h"

namespace PodModule {

PinlingImageCapability::PinlingImageCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingImageCapability 构造");
}

PinlingImageCapability::~PinlingImageCapability() {
    MYLOG_DEBUG("[品凌能力] PinlingImageCapability 析构");
}

PodResult<ImageFrame> PinlingImageCapability::captureImage() {
    MYLOG_INFO("{} 品凌图像抓拍", logPrefix());
    // TODO: 通过 Session 抓拍品凌图像
    ImageFrame frame;
    frame.width = 1920;
    frame.height = 1080;
    frame.channels = 3;
    frame.format = "JPEG";
    return PodResult<ImageFrame>::success(frame, "品凌图像抓拍成功（占位）");
}

PodResult<std::string> PinlingImageCapability::saveImage(const std::string& path) {
    MYLOG_INFO("{} 保存品凌图像到: {}", logPrefix(), path);
    return PodResult<std::string>::success(path, "品凌图像保存成功（占位）");
}

} // namespace PodModule
