#pragma once

/**
 * @file i_stream_capability.h
 * @brief 流媒体能力接口
 */

#include "i_capability.h"
#include "../../common/pod_models.h"

namespace PodModule {

/**
 * @brief 流媒体能力接口
 * 
 * 提供视频流的拉取、停止和状态查询功能。
 */
class IStreamCapability : virtual public ICapability {
public:
    ~IStreamCapability() override = default;

    /** @brief 启动拉流 */
    virtual PodResult<StreamInfo> startStream(StreamType type = StreamType::RTSP) = 0;

    /** @brief 停止拉流 */
    virtual PodResult<void> stopStream() = 0;

    /** @brief 获取当前流信息 */
    virtual PodResult<StreamInfo> getStreamInfo() = 0;

    /** @brief 是否正在拉流 */
    virtual bool isStreaming() const = 0;
};

} // namespace PodModule
