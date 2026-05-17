/**
 *******************************************************************************
 * @file    signal_validation.cpp
 * @brief   Signal profile validation implementation
 *******************************************************************************
 * @attention
 *
 * Implements validation logic for signal parameters.
 * All checks are pure functions without hardware dependencies.
 *
 *******************************************************************************
 * @note
 *
 * Validation is strict: any out-of-range parameter returns an error.
 * This prevents invalid configurations from reaching the hardware.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/05
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "domain/signal_validation.hpp"

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

/* STM32H723 DAC default limits */
constexpr uint32_t kDefaultMinFrequencyHz = 1;
constexpr uint32_t kDefaultMaxFrequencyHz = 100000;
constexpr uint32_t kDefaultMinSampleRateHz = 1000;
constexpr uint32_t kDefaultMaxSampleRateHz = 1000000;
constexpr int32_t kDefaultMinVoltageMv = 0;
constexpr int32_t kDefaultMaxVoltageMv = 3300;

/*-------- 3. implementation -----------------------------------------------------------------------------------------*/

Result<void> validate_signal_profile(
    const SignalProfile& profile,
    const SignalLimits& limits
)
{
    /* Check frequency range */
    if (profile.frequency.value < limits.min_frequency.value ||
        profile.frequency.value > limits.max_frequency.value) {
        return ErrorCode::InvalidArgument;
    }

    /* Check sample rate range */
    if (profile.sample_rate.value < limits.min_sample_rate.value ||
        profile.sample_rate.value > limits.max_sample_rate.value) {
        return ErrorCode::InvalidArgument;
    }

    /* Check amplitude range */
    if (profile.amplitude.value < limits.min_voltage.value ||
        profile.amplitude.value > limits.max_voltage.value) {
        return ErrorCode::InvalidArgument;
    }

    /* Check offset range */
    if (profile.offset.value < limits.min_voltage.value ||
        profile.offset.value > limits.max_voltage.value) {
        return ErrorCode::InvalidArgument;
    }

    /* Check amplitude + offset doesn't exceed limits */
    int32_t peak_positive = profile.offset.value + profile.amplitude.value / 2;
    int32_t peak_negative = profile.offset.value - profile.amplitude.value / 2;

    if (peak_positive > limits.max_voltage.value ||
        peak_negative < limits.min_voltage.value) {
        return ErrorCode::InvalidArgument;
    }

    /* Check duty cycle for square wave */
    if (profile.kind == WaveformKind::Square) {
        if (profile.duty.value > 1000) {
            return ErrorCode::InvalidArgument;
        }
    }

    /* Sample rate must be at least 2x frequency (Nyquist) */
    if (profile.sample_rate.value < profile.frequency.value * 2) {
        return ErrorCode::InvalidArgument;
    }

    return ErrorCode::Ok;
}

SignalLimits get_default_signal_limits()
{
    return SignalLimits{
        .min_frequency = FrequencyHz{kDefaultMinFrequencyHz},
        .max_frequency = FrequencyHz{kDefaultMaxFrequencyHz},
        .min_sample_rate = SampleRateHz{kDefaultMinSampleRateHz},
        .max_sample_rate = SampleRateHz{kDefaultMaxSampleRateHz},
        .min_voltage = VoltageMv{kDefaultMinVoltageMv},
        .max_voltage = VoltageMv{kDefaultMaxVoltageMv},
    };
}

SignalProfile get_default_signal_profile()
{
    return SignalProfile{
        .kind = WaveformKind::Sine,
        .frequency = FrequencyHz{1000},
        .sample_rate = SampleRateHz{10000},
        .amplitude = VoltageMv{1000},
        .offset = VoltageMv{1650},
        .duty = DutyPermille{500},
        .output_enabled = false,
    };
}

} // namespace omnigen