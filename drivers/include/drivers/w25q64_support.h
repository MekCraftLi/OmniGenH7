/**
 *******************************************************************************
 * @file    w25q64_support.h
 * @brief   W25Q64 board support API over STM32 OCTOSPI
 *******************************************************************************
 * @attention
 *
 * This C API wraps board-specific NOR flash access for higher-level platform
 * adapters and diagnostics.
 *
 *******************************************************************************
 * @note
 *
 * Erase operations must be sector-aligned according to the W25Q64 geometry.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */
#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct device;

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

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

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

