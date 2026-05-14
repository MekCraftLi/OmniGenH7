/**
 *******************************************************************************
 * @file    zephyr_display.cpp
 * @brief   Zephyr display adapter for ILI9481 over FMC
 *******************************************************************************
 */

#include "platform/zephyr_display.hpp"

#include "drivers/ili9481_support.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zephyr_display, CONFIG_LOG_DEFAULT_LEVEL);

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

} // namespace omnigen
