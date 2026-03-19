#pragma once

/**
 * @file dji_image_capability.h
 * @brief 大疆图像抓拍能力
 */

#include "../base/base_image_capability.h"

namespace PodModule {

class DjiImageCapability : public BaseImageCapability {
public:
    DjiImageCapability();
    ~DjiImageCapability() override;

    PodResult<ImageFrame> captureImage() override;
    PodResult<std::string> saveImage(const std::string& path) override;
};

} // namespace PodModule
