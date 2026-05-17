/**
 *******************************************************************************
 * @file    waveform_synthesis.cpp
 * @brief   Waveform synthesis implementation
 *******************************************************************************
 * @attention
 *
 * Implements waveform generation for DAC output.
 * Uses lookup tables and simple math for efficiency.
 *
 *******************************************************************************
 * @note
 *
 * All calculations use integer arithmetic for embedded compatibility.
 * Sine wave uses a 256-entry lookup table.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "domain/waveform_synthesis.hpp"

namespace omnigen {

/*-------- 2. variables ----------------------------------------------------------------------------------------------*/

/* Sine lookup table (quarter wave, 0-255 maps to 0-32767) */
static const uint16_t sine_table[64] = {
    0,  804,  1608,  2411,  3212,  4012,  4808,  5602,
    6393,  7180,  7962,  8739,  9512, 10279, 11039, 11793,
    12540, 13279, 14010, 14733, 15447, 16151, 16846, 17531,
    18205, 18868, 19520, 20160, 20788, 21403, 22006, 22595,
    23170, 23732, 24279, 24812, 25330, 25833, 26320, 26791,
    27246, 27684, 28106, 28511, 28899, 29269, 29622, 29957,
    30274, 30572, 30853, 31114, 31357, 31581, 31786, 31972,
    32138, 32286, 32413, 32522, 32610, 32679, 32729, 32758
};

/*-------- 3. implementation -----------------------------------------------------------------------------------------*/

void generate_sine(uint16_t* buffer, size_t count, uint16_t amplitude, uint16_t offset)
{
    for (size_t i = 0; i < count; i++) {
        /* Calculate phase (0-255 for full cycle) */
        uint32_t phase = (i * 256) / count;

        /* Get sine value from lookup table */
        int32_t value;
        if (phase < 64) {
            /* First quarter: 0-90 degrees */
            value = sine_table[phase];
        } else if (phase < 128) {
            /* Second quarter: 90-180 degrees */
            value = sine_table[127 - phase];
        } else if (phase < 192) {
            /* Third quarter: 180-270 degrees */
            value = -sine_table[phase - 128];
        } else {
            /* Fourth quarter: 270-360 degrees */
            value = -sine_table[255 - phase];
        }

        /* Scale to amplitude and add offset */
        int32_t sample = offset + (value * amplitude / 32768);

        /* Clamp to DAC range */
        if (sample < 0) sample = 0;
        if (sample > kDacMax) sample = kDacMax;

        buffer[i] = static_cast<uint16_t>(sample);
    }
}

void generate_square(uint16_t* buffer, size_t count, uint16_t amplitude, uint16_t offset, uint16_t duty)
{
    /* Calculate duty cycle threshold */
    size_t threshold = (count * duty) / 1000;

    /* High level */
    int32_t high_level = offset + amplitude / 2;
    if (high_level > kDacMax) high_level = kDacMax;

    /* Low level */
    int32_t low_level = offset - amplitude / 2;
    if (low_level < 0) low_level = 0;

    for (size_t i = 0; i < count; i++) {
        buffer[i] = (i < threshold) ? static_cast<uint16_t>(high_level) : static_cast<uint16_t>(low_level);
    }
}

void generate_triangle(uint16_t* buffer, size_t count, uint16_t amplitude, uint16_t offset)
{
    int32_t min_val = offset - amplitude / 2;
    int32_t max_val = offset + amplitude / 2;

    if (min_val < 0) min_val = 0;
    if (max_val > kDacMax) max_val = kDacMax;

    for (size_t i = 0; i < count; i++) {
        /* Triangle wave: ramp up then ramp down */
        int32_t phase = (i * 2 * (max_val - min_val)) / count + min_val;

        if (phase > max_val) {
            phase = 2 * max_val - phase;
        }

        buffer[i] = static_cast<uint16_t>(phase);
    }
}

void generate_sawtooth(uint16_t* buffer, size_t count, uint16_t amplitude, uint16_t offset)
{
    int32_t min_val = offset - amplitude / 2;
    int32_t max_val = offset + amplitude / 2;

    if (min_val < 0) min_val = 0;
    if (max_val > kDacMax) max_val = kDacMax;

    for (size_t i = 0; i < count; i++) {
        /* Sawtooth: linear ramp from min to max */
        int32_t sample = min_val + (i * (max_val - min_val)) / count;
        buffer[i] = static_cast<uint16_t>(sample);
    }
}

void generate_waveform(uint16_t* buffer, size_t count, const SignalProfile& profile)
{
    /* Convert voltage to DAC units */
    uint16_t amplitude = voltage_to_dac(profile.amplitude.value);
    uint16_t offset = voltage_to_dac(profile.offset.value);

    /* Amplitude is peak-to-peak, so divide by 2 */
    amplitude = amplitude / 2;

    switch (profile.kind) {
        case WaveformKind::Sine:
            generate_sine(buffer, count, amplitude, offset);
            break;

        case WaveformKind::Square:
            generate_square(buffer, count, amplitude, offset, profile.duty.value);
            break;

        case WaveformKind::Triangle:
            generate_triangle(buffer, count, amplitude, offset);
            break;

        case WaveformKind::Sawtooth:
            generate_sawtooth(buffer, count, amplitude, offset);
            break;

        default:
            /* Default to sine wave */
            generate_sine(buffer, count, amplitude, offset);
            break;
    }
}

uint16_t voltage_to_dac(int32_t voltage_mv, int32_t vref_mv)
{
    if (voltage_mv < 0) return 0;
    if (voltage_mv >= vref_mv) return kDacMax;

    return static_cast<uint16_t>((voltage_mv * kDacMax) / vref_mv);
}

} // namespace omnigen
