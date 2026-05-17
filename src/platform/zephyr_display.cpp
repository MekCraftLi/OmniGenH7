/**
 *******************************************************************************
 * @file    zephyr_display.cpp
 * @brief   Zephyr display adapter implementation for ILI9481 over FMC
 *******************************************************************************
 * @attention
 *
 * This file translates platform display operations to the board-level ILI9481
 * support driver.
 *
 *******************************************************************************
 * @note
 *
 * Display initialization must run after MPU memory attributes are ready.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */
/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "platform/zephyr_display.hpp"

#include "drivers/ili9481_support.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zephyr_display, CONFIG_LOG_DEFAULT_LEVEL);

/*-------- 3. implementation -----------------------------------------------------------------------------------------*/

namespace omnigen {

static Result<void> map_errno_to_result(int ret)
{
    if (ret == 0) {
        return ErrorCode::Ok;
    }

    switch (ret) {
        case -EINVAL:
            return ErrorCode::InvalidArgument;
        case -ENODEV:
            return ErrorCode::InvalidState;
        case -EACCES:
            return ErrorCode::InvalidState;
        case -EBUSY:
            return ErrorCode::Busy;
        case -ETIMEDOUT:
            return ErrorCode::Timeout;
        default:
            return ErrorCode::IoError;
    }
}

Result<void> ZephyrDisplay::mount()
{
    const int ret = ili9481_support_init();
    if (ret != 0) {
        LOG_ERR("ILI9481 init failed: %d", ret);
        return map_errno_to_result(ret);
    }

    mounted_ = true;
    return ErrorCode::Ok;
}

Result<void> ZephyrDisplay::clear(uint16_t rgb565)
{
    const int ret = ili9481_support_clear(rgb565);
    if (ret != 0) {
        LOG_ERR("ILI9481 clear failed: %d", ret);
        return map_errno_to_result(ret);
    }

    return ErrorCode::Ok;
}

Result<void> ZephyrDisplay::fill(const DisplayRect& rect, uint16_t rgb565)
{
    const int ret = ili9481_support_fill_rect(rect.x, rect.y, rect.width, rect.height, rgb565);
    if (ret != 0) {
        LOG_ERR("ILI9481 fill failed: %d", ret);
        return map_errno_to_result(ret);
    }

    return ErrorCode::Ok;
}

Result<void> ZephyrDisplay::blit(const DisplayBlitRequest& request)
{
    const auto& rect = request.rect;
    const int ret = ili9481_support_blit_rgb565(rect.x, rect.y, rect.width, rect.height, request.pixels);
    if (ret != 0) {
        LOG_ERR("ILI9481 blit failed: %d", ret);
        return map_errno_to_result(ret);
    }

    return ErrorCode::Ok;
}

} // namespace omnigen
