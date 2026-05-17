/**
 *******************************************************************************
 * @file    filter_switch_port.hpp
 * @brief   Filter bank switch abstraction for analog output conditioning
 *******************************************************************************
 * @attention
 *
 * FilterSwitchPort is the platform boundary for selecting the analog output
 * reconstruction filter and muting the output during switching.
 *
 *******************************************************************************
 * @note
 *
 * Filter modes follow the hardware filter bank documented in the board handoff:
 * bypass, 25 kHz low-pass, 125 kHz low-pass, and 225 kHz low-pass.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "base/result.hpp"

#include <cstdint>

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

enum class FilterMode : uint8_t {
    Bypass,
    Low25k,
    Mid125k,
    High225k,
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

class FilterSwitchPort {
public:
    virtual ~FilterSwitchPort() = default;

    virtual Result<void> mount() = 0;
    virtual Result<void> set_mode(FilterMode mode) = 0;
    virtual Result<void> set_mute(bool enabled) = 0;
    virtual bool mounted() const = 0;
    virtual bool muted() const = 0;
    virtual FilterMode mode() const = 0;
};

} // namespace omnigen
