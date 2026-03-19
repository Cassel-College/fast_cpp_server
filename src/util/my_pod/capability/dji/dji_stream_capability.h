#pragma once

/**
 * @file dji_stream_capability.h
 * @brief 大疆流媒体能力
 */

#include "../base/base_stream_capability.h"

namespace PodModule {

class DjiStreamCapability : public BaseStreamCapability {
public:
    DjiStreamCapability();
    ~DjiStreamCapability() override;

    PodResult<StreamInfo> startStream(StreamType type = StreamType::RTSP) override;
    PodResult<void> stopStream() override;
};

} // namespace PodModule
