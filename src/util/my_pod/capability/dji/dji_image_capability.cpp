/**
 * @file dji_image_capability.cpp
 * @brief 大疆图像抓拍能力实现
 */

#include "dji_image_capability.h"
#include "MyLog.h"

namespace PodModule {

DjiImageCapability::DjiImageCapability() {
    MYLOG_DEBUG("[大疆能力] DjiImageCapability 构造");
}

DjiImageCapability::~DjiImageCapability() {
    MYLOG_DEBUG("[大疆能力] DjiImageCapability 析构");
}

PodResult<ImageFrame> DjiImageCapability::captureImage() {
    MYLOG_INFO("{} 大疆图像抓拍", logPrefix());
    // TODO: 通过 Session 抓拍大疆图像
    ImageFrame frame;
    frame.width = 3840;
    frame.height = 2160;
    frame.channels = 3;
    frame.format = "JPEG";
    return PodResult<ImageFrame>::success(frame, "大疆图像抓拍成功（占位）");
}

PodResult<std::string> DjiImageCapability::saveImage(const std::string& path) {
    MYLOG_INFO("{} 保存大疆图像到: {}", logPrefix(), path);
    // TODO: 实际保存逻辑
    return PodResult<std::string>::success(path, "大疆图像保存成功（占位）");
}

} // namespace PodModule
