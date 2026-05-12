/**
 *******************************************************************************
 * @file    w25q64_support.h
 * @brief   W25Q64 support layer over Zephyr flash API
 *******************************************************************************
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct device;

#ifdef __cplusplus
extern "C" {
#endif

int w25q64_support_init(void);
bool w25q64_support_ready(void);
const struct device* w25q64_support_device(void);

int w25q64_support_read_jedec_id(uint8_t id[3]);
int w25q64_support_get_size(uint32_t* out_size);
size_t w25q64_support_get_write_block_size(void);

int w25q64_support_read(uint32_t address, void* data, size_t len);
int w25q64_support_write(uint32_t address, const void* data, size_t len);
int w25q64_support_erase_sector(uint32_t address);
int w25q64_support_erase_range(uint32_t address, size_t len);

#ifdef __cplusplus
}
#endif

