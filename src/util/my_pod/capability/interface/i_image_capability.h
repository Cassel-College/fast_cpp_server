#pragma once

/**
 * @file i_image_capability.h
 * @brief 图像抓拍能力接口
 */

#include "i_capability.h"
#include "../../common/pod_models.h"

namespace PodModule {

/**
 * @brief 图像抓拍能力接口
 * 
 * 提供图像抓拍和保存功能。
 */
class IImageCapability : virtual public ICapability {
public:
    ~IImageCapability() override = default;

    /** @brief 抓拍一帧图像 */
    virtual PodResult<ImageFrame> captureImage() = 0;

    /** @brief 将图像保存到文件 */
    virtual PodResult<std::string> saveImage(const std::string& path) = 0;
};

} // namespace PodModule
