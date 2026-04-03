#pragma once

/**
 * @file AudioDto.hpp
 * @brief 扬声器模块 API 的 DTO 定义
 *
 * 包含音频接口用到的请求/响应数据传输对象：
 * - AudioDeviceIdDto     : 通过 device_id 定位扬声器
 * - AudioPlayRequestDto  : 指定设备播放音频文件的请求
 * - AudioVolumeRequestDto: 设置指定设备音量的请求
 */

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

namespace my_api::dto {

#include OATPP_CODEGEN_BEGIN(DTO)

/**
 * @brief 扬声器设备 ID DTO
 *
 * 用于需要指定单个扬声器设备的请求。
 */
class AudioDeviceIdDto : public oatpp::DTO {
    DTO_INIT(AudioDeviceIdDto, DTO)

    /** 设备唯一标识 */
    DTO_FIELD(String, device_id);
};

/**
 * @brief 播放音频文件请求 DTO
 *
 * 指定在某个扬声器上播放指定路径的音频文件。
 */
class AudioPlayRequestDto : public oatpp::DTO {
    DTO_INIT(AudioPlayRequestDto, DTO)

    /** 设备唯一标识 */
    DTO_FIELD(String, device_id);

    /** 音频文件路径（服务器本地路径） */
    DTO_FIELD(String, filepath);
};

/**
 * @brief 设置音量请求 DTO
 *
 * 设置指定扬声器的音量。
 */
class AudioVolumeRequestDto : public oatpp::DTO {
    DTO_INIT(AudioVolumeRequestDto, DTO)

    /** 设备唯一标识 */
    DTO_FIELD(String, device_id);

    /** 音量值（0-100） */
    DTO_FIELD(Int32, volume);
};

#include OATPP_CODEGEN_END(DTO)

} // namespace my_api::dto
