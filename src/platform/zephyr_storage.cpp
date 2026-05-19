/**
 *******************************************************************************
 * @file    zephyr_storage.cpp
 * @brief   Zephyr storage adapter implementation for W25Q64
 *******************************************************************************
 * @attention
 *
 * This file forwards StoragePort operations to the board-level W25Q64 support
 * driver.
 *
 *******************************************************************************
 * @note
 *
 * Address ranges are validated by the lower-level storage support driver.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */
/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "platform/zephyr_storage.hpp"

#include "drivers/w25q64_support.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zephyr_storage, CONFIG_LOG_DEFAULT_LEVEL);

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
        case -EBUSY:
            return ErrorCode::Busy;
        case -ETIMEDOUT:
            return ErrorCode::Timeout;
        default:
            return ErrorCode::IoError;
    }
}

Result<void> ZephyrStorage::initialize()
{
    int ret = w25q64_support_init();
    if (ret != 0) {
        LOG_ERR("W25Q64 init failed: %d", ret);
        return map_errno_to_result(ret);
    }

#if defined(CONFIG_FLASH_JESD216_API)
    uint8_t jedec[3] = {0U};
    ret = w25q64_support_read_jedec_id(jedec);
    if (ret == 0) {
        LOG_INF("W25Q64 JEDEC ID: %02x %02x %02x", jedec[0], jedec[1], jedec[2]);
    } else {
        LOG_WRN("W25Q64 JEDEC ID read failed: %d", ret);
    }
#endif

    ready_ = true;
    return ErrorCode::Ok;
}

Result<void> ZephyrStorage::read(const StorageReadRequest& request)
{
    return map_errno_to_result(w25q64_support_read(request.address, request.buffer, request.length));
}

Result<void> ZephyrStorage::write(const StorageWriteRequest& request)
{
    return map_errno_to_result(w25q64_support_write(request.address, request.data, request.length));
}

Result<void> ZephyrStorage::erase_sector(uint32_t address)
{
    return map_errno_to_result(w25q64_support_erase_sector(address));
}

Result<void> ZephyrStorage::erase_range(const StorageEraseRangeRequest& request)
{
    return map_errno_to_result(w25q64_support_erase_range(request.address, request.length));
}

} // namespace omnigen
