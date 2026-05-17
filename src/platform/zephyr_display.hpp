/**
 *******************************************************************************
 * @file    zephyr_display.hpp
 * @brief   Zephyr display adapter for ILI9481 over FMC
 *******************************************************************************
 * @attention
 *
 * This adapter exposes a small C++ platform interface over the board-level C
 * ILI9481 support driver.
 *
 *******************************************************************************
 * @note
 *
 * The underlying LCD bus and reset GPIO are described in board devicetree.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */
#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "ports/display_port.hpp"

#include <cstdint>

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

namespace omnigen {

class ZephyrDisplay : public DisplayPort {
public:
    ZephyrDisplay() = default;
    ~ZephyrDisplay() override = default;

    Result<void> mount() override;
    Result<void> clear(uint16_t rgb565) override;
    Result<void> fill(const DisplayRect& rect, uint16_t rgb565) override;
    Result<void> blit(const DisplayBlitRequest& request) override;
    bool mounted() const override { return mounted_; }

private:
    bool mounted_ = false;
};

} // namespace omnigen
