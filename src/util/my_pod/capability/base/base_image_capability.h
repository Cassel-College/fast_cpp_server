#pragma once

/**
 * @file base_image_capability.h
 * @brief 基础图像抓拍能力
 */

#include "base_capability.h"
#include "../interface/i_image_capability.h"

namespace PodModule {

/**
 * @brief 基础图像抓拍能力
 */
class BaseImageCapability : public BaseCapability, public IImageCapability {
public:
    BaseImageCapability();
    ~BaseImageCapability() override;

    // --- ICapability ---
    CapabilityType getType() const override;
    std::string getName() const override;
    PodResult<void> initialize() override;
    void attachPod(IPod* pod) override { BaseCapability::attachPod(pod); }
    void attachSession(std::shared_ptr<ISession> session) override { BaseCapability::attachSession(session); }

    // --- IImageCapability ---
    PodResult<ImageFrame> captureImage() override;
    PodResult<std::string> saveImage(const std::string& path) override;
};

} // namespace PodModule
