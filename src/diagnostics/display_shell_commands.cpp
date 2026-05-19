/**
 *******************************************************************************
 * @file    display_shell_commands.cpp
 * @brief   LCD and LVGL shell diagnostics
 *******************************************************************************
 * @attention
 *
 * Provides the `lcd` and `lvgl` shell command groups for display bring-up and
 * runtime checks.
 *
 *******************************************************************************
 * @note
 *
 * LCD commands use the low-level ILI9481 support API directly. LVGL commands
 * only request work from the LVGL platform adapter; LVGL object ownership stays
 * inside the LVGL thread.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/19
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "diagnostics/shell_parse.hpp"
#include "drivers/ili9481_support.h"
#include "platform/lvgl_port.hpp"

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

namespace omnigen {

/*-------- 3. internal helpers ---------------------------------------------------------------------------------------*/

static int lcd_check_ready(const struct shell* sh)
{
    if (!ili9481_support_ready()) {
        shell_error(sh, "LCD not ready. Re-run: lcd init");
        return -ENODEV;
    }

    return 0;
}

/*-------- 4. command handlers ---------------------------------------------------------------------------------------*/

static int cmd_lcd_init(const struct shell* sh, size_t argc, char** argv)
{
    (void)argc;
    (void)argv;

    int ret = ili9481_support_init();
    if (ret != 0) {
        shell_error(sh, "LCD init failed: %d", ret);
        return ret;
    }

    shell_print(sh, "LCD initialized");
    return 0;
}

static int cmd_lcd_status(const struct shell* sh, size_t argc, char** argv)
{
    (void)argc;
    (void)argv;

    const bool ready = ili9481_support_ready();
    shell_print(sh, "LCD status: %s", ready ? "ready" : "not ready");
    if (!ready) {
        shell_print(sh, "Run: lcd init");
    }

    return 0;
}

static int cmd_lcd_clear(const struct shell* sh, size_t argc, char** argv)
{
    if (argc != 2U) {
        shell_error(sh, "Usage: lcd clear <color565>");
        return -EINVAL;
    }

    int ret = lcd_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint16_t color = 0U;
    if (!parse_shell_u16_arg(argv[1], &color)) {
        shell_error(sh, "Invalid color565: %s", argv[1]);
        return -EINVAL;
    }

    ret = ili9481_support_clear(color);
    if (ret != 0) {
        shell_error(sh, "LCD clear failed: %d", ret);
        return ret;
    }

    shell_print(sh, "LCD clear done: color=0x%04x", color);
    return 0;
}

static int cmd_lcd_fill(const struct shell* sh, size_t argc, char** argv)
{
    if (argc != 6U) {
        shell_error(sh, "Usage: lcd fill <x> <y> <w> <h> <color565>");
        return -EINVAL;
    }

    int ret = lcd_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint16_t x     = 0U;
    uint16_t y     = 0U;
    uint16_t w     = 0U;
    uint16_t h     = 0U;
    uint16_t color = 0U;
    if (!parse_shell_u16_arg(argv[1], &x)) {
        shell_error(sh, "Invalid x: %s", argv[1]);
        return -EINVAL;
    }
    if (!parse_shell_u16_arg(argv[2], &y)) {
        shell_error(sh, "Invalid y: %s", argv[2]);
        return -EINVAL;
    }
    if (!parse_shell_u16_arg(argv[3], &w) || w == 0U) {
        shell_error(sh, "Invalid w: %s", argv[3]);
        return -EINVAL;
    }
    if (!parse_shell_u16_arg(argv[4], &h) || h == 0U) {
        shell_error(sh, "Invalid h: %s", argv[4]);
        return -EINVAL;
    }
    if (!parse_shell_u16_arg(argv[5], &color)) {
        shell_error(sh, "Invalid color565: %s", argv[5]);
        return -EINVAL;
    }

    ret = ili9481_support_fill_rect(x, y, w, h, color);
    if (ret != 0) {
        shell_error(sh, "LCD fill failed: %d", ret);
        return ret;
    }

    shell_print(sh, "LCD fill done: x=%u y=%u w=%u h=%u color=0x%04x", x, y, w, h, color);
    return 0;
}

static int cmd_lcd_test(const struct shell* sh, size_t argc, char** argv)
{
    (void)argc;
    (void)argv;

    int ret = lcd_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    static const uint16_t colors[] = {
        0xF800U, /* red */
        0x07E0U, /* green */
        0x001FU, /* blue */
        0xFFFFU, /* white */
        0x0000U, /* black */
    };

    shell_print(sh, "LCD test start");
    for (size_t i = 0U; i < (sizeof(colors) / sizeof(colors[0])); ++i) {
        ret = ili9481_support_clear(colors[i]);
        if (ret != 0) {
            shell_error(sh, "LCD test failed at step %u: %d", static_cast<unsigned int>(i), ret);
            return ret;
        }
        k_msleep(250);
    }

    ret = ili9481_support_fill_rect(0U, 0U, 80U, 80U, 0xF800U);
    if (ret != 0) {
        shell_error(sh, "LCD test fill #1 failed: %d", ret);
        return ret;
    }
    ret = ili9481_support_fill_rect(80U, 0U, 80U, 80U, 0x07E0U);
    if (ret != 0) {
        shell_error(sh, "LCD test fill #2 failed: %d", ret);
        return ret;
    }
    ret = ili9481_support_fill_rect(160U, 0U, 80U, 80U, 0x001FU);
    if (ret != 0) {
        shell_error(sh, "LCD test fill #3 failed: %d", ret);
        return ret;
    }

    shell_print(sh, "LCD test done");
    return 0;
}

static int cmd_lvgl_status(const struct shell* sh, size_t argc, char** argv)
{
    (void)argc;
    (void)argv;

    shell_print(sh, "LVGL status:");
    shell_print(sh, "  ready: %s", lvgl_port_ready() ? "yes" : "no");
    shell_print(sh, "  stress: %s", lvgl_port_stress_test_requested() ? "running/requested" : "idle");
    return 0;
}

static int cmd_lvgl_stress(const struct shell* sh, size_t argc, char** argv)
{
    (void)argc;
    (void)argv;

    auto result = lvgl_port_start_stress_test();
    if (result.is_error()) {
        shell_error(sh, "LVGL stress start failed: %d", static_cast<int>(result.error()));
        shell_error(sh, "Make sure the display is initialized and LVGL is ready");
        return -EIO;
    }

    shell_print(sh, "LVGL stress test requested");
    return 0;
}

/*-------- 5. shell command definitions ------------------------------------------------------------------------------*/

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_lcd,
    SHELL_CMD(init, NULL, "Initialize ILI9481 panel", cmd_lcd_init),
    SHELL_CMD(status, NULL, "Show LCD init status", cmd_lcd_status),
    SHELL_CMD(clear, NULL, "Clear screen: lcd clear <color565>", cmd_lcd_clear),
    SHELL_CMD(fill, NULL, "Fill rect: lcd fill <x> <y> <w> <h> <color565>", cmd_lcd_fill),
    SHELL_CMD(test, NULL, "Run LCD color and block test", cmd_lcd_test),
    SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_lvgl,
    SHELL_CMD(status, NULL, "Show LVGL runtime status", cmd_lvgl_status),
    SHELL_CMD(stress, NULL, "Start LVGL stress demo", cmd_lvgl_stress),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(lcd, &sub_lcd, "ILI9481 LCD commands", NULL);
SHELL_CMD_REGISTER(lvgl, &sub_lvgl, "LVGL GUI commands", NULL);

} // namespace omnigen
