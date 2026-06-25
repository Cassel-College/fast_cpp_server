#pragma once

/**
 * @file LightDto.hpp
 * @brief 探照灯模块 API 的 DTO 定义
 *
 * 包含探照灯接口用到的请求数据传输对象：
 * - LightLevelDto     : 设置 LED 亮度级别
 * - LightFlashHzDto   : 设置频闪频率
 * - LightMoveToDto    : 云台移动到绝对坐标
 * - LightDirectionDto : 云台按方向移动
 */

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

namespace my_api::dto {

#include OATPP_CODEGEN_BEGIN(DTO)

/**
 * @brief 设置 LED 亮度请求 DTO
 *
 * 设置探照灯 LED 亮度等级（0～100）。
 */
class LightLevelDto : public oatpp::DTO {
    DTO_INIT(LightLevelDto, DTO)

    /** 亮度级别，范围 0～100 */
    DTO_FIELD(Int32, level);
};

/**
 * @brief 设置频闪频率请求 DTO
 *
 * 设置探照灯频闪模式的频率（1～50 Hz）。
 */
class LightFlashHzDto : public oatpp::DTO {
    DTO_INIT(LightFlashHzDto, DTO)

    /** 频闪频率，单位 Hz，范围 1～50 */
    DTO_FIELD(Int32, hz);
};

/**
 * @brief 云台绝对位置移动请求 DTO
 *
 * 控制探照灯云台移动到指定绝对角度坐标。
 */
class LightMoveToDto : public oatpp::DTO {
    DTO_INIT(LightMoveToDto, DTO)

    /** 水平角度，范围 -180～180 */
    DTO_FIELD(Int32, x);

    /** 垂直角度，范围 -180～180 */
    DTO_FIELD(Int32, y);
};

/**
 * @brief 云台方向移动请求 DTO
 *
 * 控制探照灯云台向指定方向移动固定步长（5°）。
 * direction 取值：up / down / left / right
 */
class LightDirectionDto : public oatpp::DTO {
    DTO_INIT(LightDirectionDto, DTO)

    /** 移动方向：up / down / left / right */
    DTO_FIELD(String, direction);
};

#include OATPP_CODEGEN_END(DTO)

} // namespace my_api::dto
