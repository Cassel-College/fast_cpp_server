#pragma once

/**
 * @file base_stream_capability.h
 * @brief 基础流媒体能力
 */

#include "base_capability.h"
#include "../interface/i_stream_capability.h"
#include <atomic>

namespace PodModule {

/**
 * @brief 基础流媒体能力
 */
class BaseStreamCapability : public BaseCapability, public IStreamCapability {
public:
    BaseStreamCapability();
    ~BaseStreamCapability() override;

    // --- ICapability ---
    CapabilityType getType() const override;
    std::string getName() const override;
    PodResult<void> initialize() override;
    void attachPod(IPod* pod) override { BaseCapability::attachPod(pod); }
    void attachSession(std::shared_ptr<ISession> session) override { BaseCapability::attachSession(session); }

    // --- IStreamCapability ---
    PodResult<StreamInfo> startStream(StreamType type = StreamType::RTSP) override;
    PodResult<void> stopStream() override;
    PodResult<StreamInfo> getStreamInfo() override;
    bool isStreaming() const override;

protected:
    std::atomic<bool> streaming_{false};  // 是否正在推流
    StreamInfo current_stream_;            // 当前流信息
};

} // namespace PodModule
