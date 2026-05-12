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

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "base/units.hpp"

namespace omnigen {

/* ------- enum ------------------------------------------------------------------------------------------------------*/

/**
 * @brief Waveform type enumeration.
 */
enum class WaveformKind : uint8_t {
    None,
    Sine,
    Square,
    Pwm,
    Triangle,
    Sawtooth,
    Arbitrary,
};

/* ------- class prototypes ------------------------------------------------------------------------------------------*/

/**
 * @brief Signal output configuration.
 */
struct SignalProfile {
    WaveformKind kind;
    FrequencyHz frequency;
    SampleRateHz sample_rate;
    uint16_t samples_per_cycle;
    VoltageMv amplitude;
    VoltageMv offset;
    DutyPermille duty;
    bool output_enabled;
};

/**
 * @brief Signal parameter limits.
 */
struct SignalLimits {
    FrequencyHz min_frequency;
    FrequencyHz max_frequency;
    SampleRateHz min_sample_rate;
    SampleRateHz max_sample_rate;
    VoltageMv min_voltage;
    VoltageMv max_voltage;
    uint16_t min_samples_per_cycle;
    uint16_t max_samples_per_cycle;
};

} // namespace omnigen
