#pragma once

/**
 * @file pinling_stream_capability.h
 * @brief 品凌流媒体能力
 */

#include "../base/base_stream_capability.h"

namespace PodModule {

class PinlingStreamCapability : public BaseStreamCapability {
public:
    PinlingStreamCapability();
    ~PinlingStreamCapability() override;

    PodResult<StreamInfo> startStream(StreamType type = StreamType::RTSP) override;
    PodResult<void> stopStream() override;
};

} // namespace PodModule
