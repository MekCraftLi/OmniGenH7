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

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "domain/signal_validation.hpp"

namespace omnigen {

/* ------- define ----------------------------------------------------------------------------------------------------*/

/* STM32H723 DAC default limits */
constexpr uint32_t kDefaultMinFrequencyHz = 1;
constexpr uint32_t kDefaultMaxFrequencyHz = 100000;
constexpr uint32_t kDefaultMinSampleRateHz = 1000;
constexpr uint32_t kDefaultMaxSampleRateHz = 1000000;
constexpr int32_t kDefaultMinVoltageMv = 0;
constexpr int32_t kDefaultMaxVoltageMv = 3300;
constexpr uint32_t kMilliHzPerHz = 1000U;
constexpr uint16_t kDefaultMinSamplesPerCycle = 16U;
constexpr uint16_t kDefaultMaxSamplesPerCycle = 64U;

/* ------- function implement ----------------------------------------------------------------------------------------*/

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

    if (profile.samples_per_cycle < limits.min_samples_per_cycle ||
        profile.samples_per_cycle > limits.max_samples_per_cycle) {
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

    /* Check duty cycle for duty-based waveforms */
    if (profile.kind == WaveformKind::Square || profile.kind == WaveformKind::Pwm) {
        if (profile.duty.value > 1000) {
            return ErrorCode::InvalidArgument;
        }
    }

    /* Sample rate must be at least 2x frequency (Nyquist) */
    const uint64_t nyquist_sample_rate_mhz = static_cast<uint64_t>(profile.sample_rate.value) * kMilliHzPerHz;
    if (nyquist_sample_rate_mhz < static_cast<uint64_t>(profile.frequency.value) * 2U) {
        return ErrorCode::InvalidArgument;
    }

    return ErrorCode::Ok;
}

SignalLimits get_default_signal_limits()
{
    return SignalLimits{
        .min_frequency = FrequencyHz{kDefaultMinFrequencyHz * kMilliHzPerHz},
        .max_frequency = FrequencyHz{kDefaultMaxFrequencyHz * kMilliHzPerHz},
        .min_sample_rate = SampleRateHz{kDefaultMinSampleRateHz},
        .max_sample_rate = SampleRateHz{kDefaultMaxSampleRateHz},
        .min_voltage = VoltageMv{kDefaultMinVoltageMv},
        .max_voltage = VoltageMv{kDefaultMaxVoltageMv},
        .min_samples_per_cycle = kDefaultMinSamplesPerCycle,
        .max_samples_per_cycle = kDefaultMaxSamplesPerCycle,
    };
}

SignalProfile get_default_signal_profile()
{
    return SignalProfile{
        .kind = WaveformKind::Sine,
        .frequency = FrequencyHz{1000U * kMilliHzPerHz},
        .sample_rate = SampleRateHz{64000},
        .samples_per_cycle = 64U,
        .amplitude = VoltageMv{1000},
        .offset = VoltageMv{1650},
        .duty = DutyPermille{500},
        .output_enabled = false,
    };
}

uint16_t select_samples_per_cycle(FrequencyHz frequency, const SignalLimits& limits)
{
    static constexpr uint16_t candidates[] = {64U, 32U, 16U};

    for (uint16_t samples : candidates) {
        if (samples < limits.min_samples_per_cycle || samples > limits.max_samples_per_cycle) {
            continue;
        }

        const SampleRateHz sample_rate = calculate_sample_rate(frequency, samples);
        if (sample_rate.value >= limits.min_sample_rate.value && sample_rate.value <= limits.max_sample_rate.value) {
            return samples;
        }
    }

    return limits.min_samples_per_cycle;
}

SampleRateHz calculate_sample_rate(FrequencyHz frequency, uint16_t samples_per_cycle)
{
    const uint64_t sample_rate_mhz = static_cast<uint64_t>(frequency.value) * samples_per_cycle;
    const uint32_t sample_rate_hz = static_cast<uint32_t>((sample_rate_mhz + 999U) / kMilliHzPerHz);
    return SampleRateHz{sample_rate_hz};
}

} // namespace omnigen
