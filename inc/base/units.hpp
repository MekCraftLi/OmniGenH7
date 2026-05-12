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
 * @brief Frequency in milli-Hertz.
 *
 * Shell/UI still use Hz. Internally, value stores Hz * 1000 so fractional Hz
 * can be represented without keeping floating point in the domain layer.
 */
struct FrequencyHz {
    uint32_t value;
};

/**
 * @brief Voltage in millivolts (signed for offset support).
 */
struct VoltageMv {
    int32_t value;
};

/**
 * @brief Sample rate in Hertz.
 */
struct SampleRateHz {
    uint32_t value;
};

/**
 * @brief Time duration in milliseconds.
 */
struct DurationMs {
    uint32_t value;
};

/**
 * @brief Time duration in microseconds.
 */
struct DurationUs {
    uint32_t value;
};

/**
 * @brief Duty cycle in permille (0-1000 = 0%-100%).
 */
struct DutyPermille {
    uint16_t value;
};

/**
 * @brief Phase offset in degrees (0-359).
 */
struct PhaseDegree {
    uint16_t value;
};

} // namespace omnigen
