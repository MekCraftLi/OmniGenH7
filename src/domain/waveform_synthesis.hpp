/**
 *******************************************************************************
 * @file    waveform_synthesis.hpp
 * @brief   Waveform synthesis functions
 *******************************************************************************
 * @attention
 *
 * Generate sample data for different waveform types:
 * - Sine wave
 * - Square wave
 * - Triangle wave
 * - Sawtooth wave
 *
 *******************************************************************************
 * @note
 *
 * All functions output 12-bit DAC values (0-4095).
 * Center value is 2048 (mid-scale).
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "domain/signal_profile.hpp"

#include <cstdint>
#include <stddef.h>

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

/* DAC 12-bit range */
constexpr uint16_t kDacMax = 4095;
constexpr uint16_t kDacMid = 2048;
constexpr uint16_t kDacMin = 0;

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief Generate sine wave samples.
 * @param buffer Output buffer for samples.
 * @param count Number of samples to generate.
 * @param amplitude Amplitude in DAC units (0-2047).
 * @param offset Offset in DAC units (0-4095).
 */
void generate_sine(uint16_t* buffer, size_t count, uint16_t amplitude, uint16_t offset);

/**
 * @brief Generate square wave samples.
 * @param buffer Output buffer for samples.
 * @param count Number of samples to generate.
 * @param amplitude Amplitude in DAC units.
 * @param offset Offset in DAC units.
 * @param duty Duty cycle in permille (0-1000).
 */
void generate_square(uint16_t* buffer, size_t count, uint16_t amplitude, uint16_t offset, uint16_t duty);

/**
 * @brief Generate triangle wave samples.
 * @param buffer Output buffer for samples.
 * @param count Number of samples to generate.
 * @param amplitude Amplitude in DAC units.
 * @param offset Offset in DAC units.
 */
void generate_triangle(uint16_t* buffer, size_t count, uint16_t amplitude, uint16_t offset);

/**
 * @brief Generate sawtooth wave samples.
 * @param buffer Output buffer for samples.
 * @param count Number of samples to generate.
 * @param amplitude Amplitude in DAC units.
 * @param offset Offset in DAC units.
 */
void generate_sawtooth(uint16_t* buffer, size_t count, uint16_t amplitude, uint16_t offset);

/**
 * @brief Generate waveform based on profile.
 * @param buffer Output buffer for samples.
 * @param count Number of samples to generate.
 * @param profile Signal profile with waveform type and parameters.
 */
void generate_waveform(uint16_t* buffer, size_t count, const SignalProfile& profile);

/**
 * @brief Convert voltage (mV) to DAC value.
 * @param voltage_mv Voltage in millivolts.
 * @param vref_mv Reference voltage in millivolts (typically 3300).
 * @return DAC value (0-4095).
 */
uint16_t voltage_to_dac(int32_t voltage_mv, int32_t vref_mv = 3300);

} // namespace omnigen
