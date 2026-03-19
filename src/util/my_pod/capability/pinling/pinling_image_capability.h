#pragma once

/**
 * @file pinling_image_capability.h
 * @brief 品凌图像抓拍能力
 */

#include "../base/base_image_capability.h"

namespace PodModule {

class PinlingImageCapability : public BaseImageCapability {
public:
    PinlingImageCapability();
    ~PinlingImageCapability() override;

    PodResult<ImageFrame> captureImage() override;
    PodResult<std::string> saveImage(const std::string& path) override;
};

} // namespace PodModule
