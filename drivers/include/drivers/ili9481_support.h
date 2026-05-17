/**
 *******************************************************************************
 * @file    ili9481_support.h
 * @brief   ILI9481 support layer over FMC 8080 16-bit bus
 *******************************************************************************
 * @attention
 *
 * This C API is board-specific and depends on the custom ILI9481 FMC devicetree
 * node.
 *
 *******************************************************************************
 * @note
 *
 * Pixel data is expected in RGB565 format.
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

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

int ili9481_support_init(void);
bool ili9481_support_ready(void);

int ili9481_support_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
int ili9481_support_blit_rgb565(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* pixels);

int ili9481_support_clear(uint16_t color);

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif
