/**
 *******************************************************************************
 * @file    zephyr_storage.cpp
 * @brief   Zephyr implementation of StoragePort using W25Q64
 *******************************************************************************
 */

#include "platform/zephyr_storage.hpp"

#include "drivers/w25q64_support.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zephyr_storage, CONFIG_LOG_DEFAULT_LEVEL);

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

Result<void> ZephyrStorage::mount()
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

    return ErrorCode::Ok;
}

Result<void> ZephyrStorage::read(uint32_t address, void* data, size_t len)
{
    return map_errno_to_result(w25q64_support_read(address, data, len));
}

Result<void> ZephyrStorage::write(uint32_t address, const void* data, size_t len)
{
    return map_errno_to_result(w25q64_support_write(address, data, len));
}

Result<void> ZephyrStorage::erase_sector(uint32_t address)
{
    return map_errno_to_result(w25q64_support_erase_sector(address));
}

Result<void> ZephyrStorage::erase_range(uint32_t address, size_t len)
{
    return map_errno_to_result(w25q64_support_erase_range(address, len));
}

} // namespace omnigen

