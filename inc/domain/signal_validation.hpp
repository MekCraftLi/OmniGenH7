/**
 *******************************************************************************
 * @file    signal_validation.hpp
 * @brief   Signal profile validation functions
 *******************************************************************************
 * @attention
 *
 * Validation functions check if signal parameters are within
 * hardware and safety limits before configuration is applied.
 *
 * All validation is done in domain layer without hardware access.
 *
 *******************************************************************************
 * @note
 *
 * Validation returns detailed error codes to help UI show
 * meaningful error messages to the user.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/05
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "signal_profile.hpp"
#include "base/result.hpp"

namespace omnigen {

/* ------- function declare ------------------------------------------------------------------------------------------*/

/**
 * @brief Validate a signal profile against limits.
 *
 * Checks:
 * - Frequency within min/max range
 * - Sample rate within min/max range
 * - Amplitude within voltage range
 * - Offset within voltage range (considering amplitude)
 * - Duty cycle valid for waveform type
 *
 * @param profile The profile to validate.
 * @param limits The limits to validate against.
 * @return Result<void> Ok if valid, error code if invalid.
 */
[[nodiscard]] Result<void> validate_signal_profile(
    const SignalProfile& profile,
    const SignalLimits& limits
);

/**
 * @brief Get default signal limits for STM32H723 DAC.
 *
 * These are conservative limits based on DAC specifications
 * and system design constraints.
 *
 * @return SignalLimits The default limits.
 */
[[nodiscard]] SignalLimits get_default_signal_limits();

/**
 * @brief Get a default/initial signal profile.
 *
 * @return SignalProfile A safe default profile.
 */
[[nodiscard]] SignalProfile get_default_signal_profile();

} // namespace omnigen