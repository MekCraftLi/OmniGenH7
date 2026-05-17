/**
 *******************************************************************************
 * @file    w25q64_support.c
 * @brief   W25Q64 support layer using Zephyr flash device API
 *******************************************************************************
 * @attention
 *
 * This file wraps the board OCTOSPI NOR flash node for diagnostics and platform
 * storage adapters.
 *
 *******************************************************************************
 * @note
 *
 * Writes are split at page boundaries; erases are delegated to the Zephyr flash
 * driver and must follow device erase alignment requirements.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */
/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "drivers/w25q64_support.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

#define W25Q64_SECTOR_SIZE 4096U
#define W25Q64_PAGE_SIZE   256U

/*
 * Requires a node label in DTS:
 *   w25q64: qspi-nor-flash@0 { ... };
 */
#define W25Q64_NODE DT_NODELABEL(w25q64)

LOG_MODULE_REGISTER(w25q64_support, CONFIG_LOG_DEFAULT_LEVEL);

static const struct device* g_w25q64_dev;

/*-------- 3. internal helpers ---------------------------------------------------------------------------------------*/

static bool is_address_in_range(uint32_t address, size_t len, uint32_t flash_size)
{
    uint64_t end = (uint64_t)address + (uint64_t)len;
    return (end <= (uint64_t)flash_size);
}

static size_t page_chunk_len(uint32_t address, size_t remain)
{
    uint32_t page_off = address % W25Q64_PAGE_SIZE;
    size_t page_left = W25Q64_PAGE_SIZE - page_off;
    return (remain < page_left) ? remain : page_left;
}

/*-------- 4. public api ---------------------------------------------------------------------------------------------*/

int w25q64_support_init(void)
{
    g_w25q64_dev = DEVICE_DT_GET(W25Q64_NODE);
    if (!device_is_ready(g_w25q64_dev)) {
        LOG_ERR("W25Q64 device not ready");
        g_w25q64_dev = NULL;
        return -ENODEV;
    }

    LOG_INF("W25Q64 support initialized");
    return 0;
}

bool w25q64_support_ready(void)
{
    return (g_w25q64_dev != NULL);
}

const struct device* w25q64_support_device(void)
{
    return g_w25q64_dev;
}

int w25q64_support_read_jedec_id(uint8_t id[3])
{
    if (g_w25q64_dev == NULL || id == NULL) {
        return -EINVAL;
    }

    return flash_read_jedec_id(g_w25q64_dev, id);
}

int w25q64_support_get_size(uint32_t* out_size)
{
    uint64_t size64 = 0U;
    int ret;

    if (g_w25q64_dev == NULL || out_size == NULL) {
        return -EINVAL;
    }

    ret = flash_get_size(g_w25q64_dev, &size64);
    if (ret != 0) {
        return ret;
    }

    if (size64 > UINT32_MAX) {
        return -EOVERFLOW;
    }

    *out_size = (uint32_t)size64;
    return 0;
}

size_t w25q64_support_get_write_block_size(void)
{
    if (g_w25q64_dev == NULL) {
        return 0U;
    }

    return flash_get_write_block_size(g_w25q64_dev);
}

int w25q64_support_read(uint32_t address, void* data, size_t len)
{
    uint32_t flash_size = 0U;
    int ret;

    if (g_w25q64_dev == NULL || data == NULL || len == 0U) {
        return -EINVAL;
    }

    ret = w25q64_support_get_size(&flash_size);
    if (ret != 0) {
        return ret;
    }

    if (!is_address_in_range(address, len, flash_size)) {
        return -EINVAL;
    }

    return flash_read(g_w25q64_dev, (off_t)address, data, len);
}

int w25q64_support_write(uint32_t address, const void* data, size_t len)
{
    const uint8_t* src = (const uint8_t*)data;
    uint32_t flash_size = 0U;
    size_t remain = len;
    int ret;

    if (g_w25q64_dev == NULL || data == NULL || len == 0U) {
        return -EINVAL;
    }

    ret = w25q64_support_get_size(&flash_size);
    if (ret != 0) {
        return ret;
    }

    if (!is_address_in_range(address, len, flash_size)) {
        return -EINVAL;
    }

    /*
     * Optimization:
     * Split writes by page boundary to keep operation predictable and
     * aligned with NOR page-program behavior.
     */
    while (remain > 0U) {
        size_t chunk = page_chunk_len(address, remain);
        ret = flash_write(g_w25q64_dev, (off_t)address, src, chunk);
        if (ret != 0) {
            return ret;
        }

        address += (uint32_t)chunk;
        src += chunk;
        remain -= chunk;
    }

    return 0;
}

int w25q64_support_erase_sector(uint32_t address)
{
    uint32_t flash_size = 0U;
    int ret;

    if (g_w25q64_dev == NULL) {
        return -EINVAL;
    }

    if ((address % W25Q64_SECTOR_SIZE) != 0U) {
        return -EINVAL;
    }

    ret = w25q64_support_get_size(&flash_size);
    if (ret != 0) {
        return ret;
    }

    if (!is_address_in_range(address, W25Q64_SECTOR_SIZE, flash_size)) {
        return -EINVAL;
    }

    return flash_erase(g_w25q64_dev, (off_t)address, W25Q64_SECTOR_SIZE);
}

int w25q64_support_erase_range(uint32_t address, size_t len)
{
    uint32_t flash_size = 0U;
    int ret;

    if (g_w25q64_dev == NULL || len == 0U) {
        return -EINVAL;
    }

    if (((address % W25Q64_SECTOR_SIZE) != 0U) || ((len % W25Q64_SECTOR_SIZE) != 0U)) {
        return -EINVAL;
    }

    ret = w25q64_support_get_size(&flash_size);
    if (ret != 0) {
        return ret;
    }

    if (!is_address_in_range(address, len, flash_size)) {
        return -EINVAL;
    }

    return flash_erase(g_w25q64_dev, (off_t)address, len);
}

