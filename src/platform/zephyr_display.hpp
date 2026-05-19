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

/**
 * @brief Zephyr LCD 显示适配类。
 *
 * 该类实现 `DisplayPort`，将 C++ 显示端口调用转发到底层 ILI9481/FMC C 驱动。
 * 内部记录初始化状态，便于请求总线查询显示模块可用性。
 */
class ZephyrDisplay : public DisplayPort {
public:
    ZephyrDisplay() = default;
    ~ZephyrDisplay() override = default;

    Result<void> initialize() override;
    Result<void> clear(uint16_t rgb565) override;
    Result<void> fill(const DisplayRect& rect, uint16_t rgb565) override;
    Result<void> blit(const DisplayBlitRequest& request) override;
    bool ready() const override { return ready_; }

private:
    bool ready_ = false;
};

} // namespace omnigen
