/**
 *******************************************************************************
 * @file    zephyr_filter_switch.hpp
 * @brief   Zephyr GPIO implementation of FilterSwitchPort
 *******************************************************************************
 * @attention
 *
 * This adapter drives the board filter selection GPIOs described by devicetree
 * aliases and exposes them through the FilterSwitchPort boundary.
 *
 *******************************************************************************
 * @note
 *
 * The filter switch sequence mutes the output, disables the switch, updates the
 * select lines, enables the switch, then releases mute after a settling delay.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "ports/filter_switch_port.hpp"

namespace omnigen {

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief Zephyr GPIO 滤波器切换适配类。
 *
 * 该类实现 `FilterSwitchPort`，负责按安全时序驱动滤波器选择、使能和静音 GPIO。
 * 调用方只需要选择逻辑滤波档位。
 */
class ZephyrFilterSwitch : public FilterSwitchPort {
public:
    ZephyrFilterSwitch() = default;
    ~ZephyrFilterSwitch() override = default;

    Result<void> initialize() override;
    Result<void> set_mode(FilterMode mode) override;
    Result<void> set_mute(bool enabled) override;
    bool ready() const override { return ready_; }
    bool muted() const override { return muted_; }
    FilterMode mode() const override { return mode_; }

private:
    FilterMode mode_ = FilterMode::Bypass;
    bool muted_ = false;
    bool ready_ = false;
};

} // namespace omnigen
