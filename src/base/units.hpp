/**
 *******************************************************************************
 * @file    units.hpp
 * @brief   Strong type units for OmniGen signal generator
 *******************************************************************************
 * @attention
 *
 * Strong typed units to prevent mixing different physical quantities.
 * Avoids bugs like passing frequency where voltage is expected.
 *
 *******************************************************************************
 * @note
 *
 * Each unit wraps a primitive type with semantic meaning.
 * All units are trivially copyable and have zero overhead.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/05
 * @version 1.0
 *******************************************************************************
 */

#pragma once

#include <cstdint>

namespace omnigen {

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 以赫兹为单位的频率值。
 *
 * 用于所有信号频率、边界频率和控制频率字段，避免把频率与电压、采样率等
 * 其他物理量混用。`value` 始终表示 Hz。
 */
struct FrequencyHz {
    uint32_t value; /**< 频率数值，单位 Hz。 */
};

/**
 * @brief 以毫伏为单位的电压值。
 *
 * 使用有符号整数以支持直流偏置等可能为负的逻辑电压参数。实际输出前需由
 * 硬件适配层或合成算法转换到 DAC 允许范围。
 */
struct VoltageMv {
    int32_t value; /**< 电压数值，单位 mV。 */
};

/**
 * @brief 以赫兹为单位的采样率。
 *
 * 描述波形样本被送入输出硬件的速率，区别于信号本身频率。`value` 始终表示
 * 每秒样本数。
 */
struct SampleRateHz {
    uint32_t value; /**< 采样率数值，单位 samples/s。 */
};

/**
 * @brief 以毫秒为单位的时间长度。
 *
 * 用于用户可感知的延时、超时和统计耗时，适合低频控制路径。
 */
struct DurationMs {
    uint32_t value; /**< 时间长度，单位 ms。 */
};

/**
 * @brief 以微秒为单位的时间长度。
 *
 * 用于硬件时序、短延时和采样级时间参数，适合需要比毫秒更高分辨率的路径。
 */
struct DurationUs {
    uint32_t value; /**< 时间长度，单位 us。 */
};

/**
 * @brief 千分比形式的占空比。
 *
 * 取值约定为 0 到 1000，其中 0 表示 0%，1000 表示 100%。该结构主要用于方波
 * 等需要占空比控制的波形参数。
 */
struct DutyPermille {
    uint16_t value; /**< 占空比千分比，范围 0-1000。 */
};

/**
 * @brief 以角度为单位的相位偏移。
 *
 * 用于描述周期波形的相位关系，约定有效范围为 0 到 359 度。
 */
struct PhaseDegree {
    uint16_t value; /**< 相位角度，单位 degree。 */
};

} // namespace omnigen
