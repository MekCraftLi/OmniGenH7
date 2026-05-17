/**
 *******************************************************************************
 * @file    signal_profile.hpp
 * @brief   Signal profile and waveform type definitions
 *******************************************************************************
 * @attention
 *
 * Domain types for signal generation. These types do not depend
 * on any hardware, RTOS, or platform-specific headers.
 *
 *******************************************************************************
 * @note
 *
 * SignalProfile describes the parameters for signal output.
 * SignalLimits defines the valid range for each parameter.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/05
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "base/units.hpp"

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

/**
 * @brief 波形类型枚举。
 *
 * 描述信号引擎需要合成或输出的波形族。该枚举只表达逻辑波形类型，不包含
 * 频率、幅值、偏置等具体参数。
 */
enum class WaveformKind : uint8_t {
    None,      /**< 未指定波形，用于默认值或无效状态。 */
    Sine,      /**< 正弦波。 */
    Square,    /**< 方波。 */
    Triangle,  /**< 三角波。 */
    Sawtooth,  /**< 锯齿波。 */
    Arbitrary, /**< 任意波形，通常来自存储或外部数据。 */
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 信号输出配置结构体。
 *
 * 聚合描述一次信号输出所需的全部核心参数。该结构属于纯领域数据，不依赖
 * RTOS、设备树或具体 DAC 驱动。
 */
struct SignalProfile {
    WaveformKind kind;      /**< 波形类型。 */
    FrequencyHz frequency;  /**< 目标信号频率。 */
    SampleRateHz sample_rate; /**< 波形采样率。 */
    VoltageMv amplitude;    /**< 输出幅值。 */
    VoltageMv offset;       /**< 直流偏置。 */
    DutyPermille duty;      /**< 占空比，仅部分波形使用。 */
    bool output_enabled;    /**< 是否允许实际输出到硬件。 */
};

/**
 * @brief 信号参数限制结构体。
 *
 * 定义硬件和系统策略允许的安全参数范围。校验层使用该结构判断 `SignalProfile`
 * 是否可以应用到当前平台。
 */
struct SignalLimits {
    FrequencyHz min_frequency;      /**< 最小允许信号频率。 */
    FrequencyHz max_frequency;      /**< 最大允许信号频率。 */
    SampleRateHz min_sample_rate;   /**< 最小允许采样率。 */
    SampleRateHz max_sample_rate;   /**< 最大允许采样率。 */
    VoltageMv min_voltage;          /**< 最小允许电压。 */
    VoltageMv max_voltage;          /**< 最大允许电压。 */
};

} // namespace omnigen
