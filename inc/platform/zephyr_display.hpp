/**
 *******************************************************************************
 * @file    zephyr_display.hpp
 * @brief   Zephyr display adapter for ILI9481 over FMC
 *******************************************************************************
 */

#pragma once

#include "base/result.hpp"

#include <cstdint>

namespace omnigen {

class ZephyrDisplay {
public:
    ZephyrDisplay() = default;
    ~ZephyrDisplay() = default;

    Result<void> mount();
    Result<void> clear(uint16_t rgb565);
};

} // namespace omnigen
