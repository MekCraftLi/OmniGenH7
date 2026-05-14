/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 MekCraftLi
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file    ili9481_support.c
 * @brief   ILI9481 support layer using FMC memory-mapped 8080 bus
 */

#include "drivers/ili9481_support.h"

#include <errno.h>
#include <stdbool.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ili9481_support, CONFIG_LOG_DEFAULT_LEVEL);

#define ILI9481_NODE DT_NODELABEL(ili9481)

#if !DT_NODE_EXISTS(ILI9481_NODE)
#error "Missing ili9481 node in board DTS"
#endif

/*
 * 8080 16-bit memory-mapped interface:
 * - Command write: base address (A16=0)
 * - Data write:    base + (1 << (RS line + 1))
 */
#define ILI9481_REG_ADDR DT_REG_ADDR(DT_PHANDLE(ILI9481_NODE, memory_region))
#define ILI9481_RS_LINE  DT_PROP(ILI9481_NODE, register_select_pin)
#define ILI9481_DATA_ADDR (ILI9481_REG_ADDR + (1UL << (ILI9481_RS_LINE + 1U)))

#define ILI9481_WIDTH  DT_PROP(ILI9481_NODE, width)
#define ILI9481_HEIGHT DT_PROP(ILI9481_NODE, height)

#define ILI9481_CMD_SOFT_RESET      0x01U
#define ILI9481_CMD_SLEEP_OUT       0x11U
#define ILI9481_CMD_DISPLAY_ON      0x29U
#define ILI9481_CMD_COL_ADDR_SET    0x2AU
#define ILI9481_CMD_PAGE_ADDR_SET   0x2BU
#define ILI9481_CMD_MEMORY_WRITE    0x2CU
#define ILI9481_CMD_MEM_ACCESS_CTRL 0x36U
#define ILI9481_CMD_PIXEL_FORMAT    0x3AU
#define ILI9481_CMD_POWER_SETTING   0xD0U
#define ILI9481_CMD_VCOM_CONTROL    0xD1U
#define ILI9481_CMD_POWER_NORMAL    0xD2U
#define ILI9481_CMD_PANEL_DRIVING   0xC0U
#define ILI9481_CMD_FRAME_RATE      0xC5U
#define ILI9481_CMD_GAMMA_SETTING   0xC8U

static const struct gpio_dt_spec g_reset_gpio =
    GPIO_DT_SPEC_GET_OR(ILI9481_NODE, reset_gpios, {0});

static bool g_ready;

static inline void ili9481_write_cmd(uint16_t cmd)
{
    *(volatile uint16_t*)ILI9481_REG_ADDR = cmd;
}

static inline void ili9481_write_data(uint16_t data)
{
    *(volatile uint16_t*)ILI9481_DATA_ADDR = data;
}

static void ili9481_write_u8_array(const uint8_t* data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        ili9481_write_data((uint16_t)data[i]);
    }
}

static void ili9481_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    ili9481_write_cmd(ILI9481_CMD_COL_ADDR_SET);
    ili9481_write_data((uint16_t)(x0 >> 8));
    ili9481_write_data((uint16_t)(x0 & 0xFFU));
    ili9481_write_data((uint16_t)(x1 >> 8));
    ili9481_write_data((uint16_t)(x1 & 0xFFU));

    ili9481_write_cmd(ILI9481_CMD_PAGE_ADDR_SET);
    ili9481_write_data((uint16_t)(y0 >> 8));
    ili9481_write_data((uint16_t)(y0 & 0xFFU));
    ili9481_write_data((uint16_t)(y1 >> 8));
    ili9481_write_data((uint16_t)(y1 & 0xFFU));
}

static int ili9481_reset_panel(void)
{
    int ret;

    if (g_reset_gpio.port == NULL) {
        return 0;
    }

    if (!gpio_is_ready_dt(&g_reset_gpio)) {
        LOG_ERR("ILI9481 reset GPIO not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&g_reset_gpio, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        LOG_ERR("ILI9481 reset GPIO configure failed: %d", ret);
        return ret;
    }

    k_msleep(20);

    ret = gpio_pin_set_dt(&g_reset_gpio, 1);
    if (ret != 0) {
        return ret;
    }
    k_msleep(20);

    ret = gpio_pin_set_dt(&g_reset_gpio, 0);
    if (ret != 0) {
        return ret;
    }
    k_msleep(120);

    return 0;
}

int ili9481_support_init(void)
{
    static const uint8_t power_setting[] = {0x07U, 0x42U, 0x18U};
    static const uint8_t vcom_control[] = {0x00U, 0x18U, 0x04U};
    static const uint8_t normal_power[] = {0x01U, 0x02U};
    static const uint8_t panel_driving[] = {0x10U, 0x3BU, 0x00U, 0x02U, 0x11U};
    static const uint8_t frame_rate[] = {0x07U};
    static const uint8_t gamma[] = {
        0x00U, 0x32U, 0x36U, 0x45U, 0x06U, 0x16U, 0x37U, 0x75U, 0x77U, 0x54U, 0x0CU, 0x00U};
    int ret;

    if (g_ready) {
        return 0;
    }

    ret = ili9481_reset_panel();
    if (ret != 0) {
        return ret;
    }

    ili9481_write_cmd(ILI9481_CMD_SOFT_RESET);
    k_msleep(10);

    ili9481_write_cmd(ILI9481_CMD_SLEEP_OUT);
    k_msleep(120);

    ili9481_write_cmd(ILI9481_CMD_POWER_SETTING);
    ili9481_write_u8_array(power_setting, sizeof(power_setting));

    ili9481_write_cmd(ILI9481_CMD_VCOM_CONTROL);
    ili9481_write_u8_array(vcom_control, sizeof(vcom_control));

    ili9481_write_cmd(ILI9481_CMD_POWER_NORMAL);
    ili9481_write_u8_array(normal_power, sizeof(normal_power));

    ili9481_write_cmd(ILI9481_CMD_PANEL_DRIVING);
    ili9481_write_u8_array(panel_driving, sizeof(panel_driving));

    ili9481_write_cmd(ILI9481_CMD_FRAME_RATE);
    ili9481_write_u8_array(frame_rate, sizeof(frame_rate));

    ili9481_write_cmd(ILI9481_CMD_GAMMA_SETTING);
    ili9481_write_u8_array(gamma, sizeof(gamma));

    /* Landscape + RGB order + top-to-bottom refresh. */
    ili9481_write_cmd(ILI9481_CMD_MEM_ACCESS_CTRL);
    ili9481_write_data(0x20U);

    /* RGB565, 16-bit per pixel. */
    ili9481_write_cmd(ILI9481_CMD_PIXEL_FORMAT);
    ili9481_write_data(0x55U);

    ili9481_set_window(0U, 0U, (uint16_t)(ILI9481_WIDTH - 1U), (uint16_t)(ILI9481_HEIGHT - 1U));

    ili9481_write_cmd(ILI9481_CMD_DISPLAY_ON);
    k_msleep(20);

    g_ready = true;
    LOG_INF("ILI9481 ready: reg=0x%08x data=0x%08x %ux%u", (uint32_t)ILI9481_REG_ADDR,
            (uint32_t)ILI9481_DATA_ADDR, (uint32_t)ILI9481_WIDTH, (uint32_t)ILI9481_HEIGHT);

    return 0;
}

bool ili9481_support_ready(void)
{
    return g_ready;
}

int ili9481_support_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint16_t x1;
    uint16_t y1;
    uint32_t x_end;
    uint32_t y_end;
    uint32_t count;

    if (!g_ready) {
        return -EACCES;
    }

    if ((w == 0U) || (h == 0U)) {
        return -EINVAL;
    }
    if ((x >= ILI9481_WIDTH) || (y >= ILI9481_HEIGHT)) {
        return -EINVAL;
    }

    x_end = (uint32_t)x + (uint32_t)w - 1U;
    y_end = (uint32_t)y + (uint32_t)h - 1U;
    if (x_end >= ILI9481_WIDTH) {
        x_end = (uint32_t)ILI9481_WIDTH - 1U;
    }
    if (y_end >= ILI9481_HEIGHT) {
        y_end = (uint32_t)ILI9481_HEIGHT - 1U;
    }
    x1 = (uint16_t)x_end;
    y1 = (uint16_t)y_end;

    ili9481_set_window(x, y, x1, y1);
    ili9481_write_cmd(ILI9481_CMD_MEMORY_WRITE);

    count = ((uint32_t)(x1 - x + 1U) * (uint32_t)(y1 - y + 1U));
    while (count-- > 0U) {
        ili9481_write_data(color);
    }

    return 0;
}

int ili9481_support_blit_rgb565(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* pixels)
{
    uint16_t x1;
    uint16_t y1;
    uint32_t x_end;
    uint32_t y_end;
    uint32_t count;

    if (!g_ready) {
        return -EACCES;
    }
    if ((pixels == NULL) || (w == 0U) || (h == 0U)) {
        return -EINVAL;
    }
    if ((x >= ILI9481_WIDTH) || (y >= ILI9481_HEIGHT)) {
        return -EINVAL;
    }

    x_end = (uint32_t)x + (uint32_t)w - 1U;
    y_end = (uint32_t)y + (uint32_t)h - 1U;
    if ((x_end >= ILI9481_WIDTH) || (y_end >= ILI9481_HEIGHT)) {
        return -EINVAL;
    }
    x1 = (uint16_t)x_end;
    y1 = (uint16_t)y_end;

    ili9481_set_window(x, y, x1, y1);
    ili9481_write_cmd(ILI9481_CMD_MEMORY_WRITE);

    count = (uint32_t)w * (uint32_t)h;
    for (uint32_t i = 0U; i < count; ++i) {
        ili9481_write_data(pixels[i]);
    }

    return 0;
}

int ili9481_support_clear(uint16_t color)
{
    return ili9481_support_fill_rect(0U, 0U, ILI9481_WIDTH, ILI9481_HEIGHT, color);
}
