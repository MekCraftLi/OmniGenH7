/**
 *******************************************************************************
 * @file    lvgl_port.cpp
 * @brief   LVGL platform adapter implementation for OmniGen H7
 *******************************************************************************
 * @attention
 *
 * This file binds LVGL to the board-specific ILI9481 FMC display driver.
 *
 *******************************************************************************
 * @note
 *
 * LVGL runs from a single Zephyr thread. Public control functions only set flags
 * consumed by that thread to keep LVGL access serialized.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "platform/lvgl_port.hpp"

#include "drivers/ili9481_support.h"

#include <lvgl.h>
#include <demos/stress/lv_demo_stress.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include <stdint.h>
#include <stddef.h>

LOG_MODULE_REGISTER(lvgl_port, CONFIG_LOG_DEFAULT_LEVEL);

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

static constexpr int32_t k_lcd_width               = 480;
static constexpr int32_t k_lcd_height              = 320;
static constexpr uint32_t k_lvgl_tick_period_ms    = 1U;
static constexpr uint32_t k_lvgl_handler_period_ms = 5U;
static constexpr size_t k_lvgl_draw_lines          = 20U;
static constexpr size_t k_lvgl_draw_pixels         = static_cast<size_t>(k_lcd_width) * k_lvgl_draw_lines;
static constexpr size_t k_lvgl_draw_buffer_bytes   = k_lvgl_draw_pixels * sizeof(uint16_t);
static constexpr uint32_t k_perf_hud_period_ms      = 1000U;

/**
 * @brief LVGL 显示刷新性能统计结构体。
 *
 * 该结构只在 LVGL GUI 线程和同步 flush 回调中访问，用于累计最近一个 HUD 周期内的
 * 刷新次数、像素数量和刷新耗时。
 */
struct LvglFlushStats {
    uint32_t flush_count{0U};
    uint32_t pixel_count{0U};
    uint32_t total_us{0U};
    uint32_t max_us{0U};
};

/*-------- 3. variables ----------------------------------------------------------------------------------------------*/

static lv_display_t* g_display = nullptr;
static lv_obj_t* g_perf_hud_label = nullptr;
static uint16_t g_draw_buffer_0[k_lvgl_draw_pixels] __aligned(4);
static uint16_t g_draw_buffer_1[k_lvgl_draw_pixels] __aligned(4);
static uint16_t g_flush_line_buffer[k_lcd_width] __aligned(4);
static LvglFlushStats g_flush_stats{};
static uint32_t g_frame_count;
static uint32_t g_last_hud_ms;
static atomic_t g_ready;
static atomic_t g_stress_requested;
static atomic_t g_stress_running;

K_THREAD_STACK_DEFINE(g_lvgl_thread_stack, 8192);
static struct k_thread g_lvgl_thread;
static k_tid_t g_lvgl_thread_id = nullptr;

/*-------- 4. implementation -----------------------------------------------------------------------------------------*/

static uint32_t lvgl_tick_get_cb()
{
    return static_cast<uint32_t>(k_uptime_get_32());
}

static void lvgl_delay_cb(uint32_t ms)
{
    k_msleep(ms);
}

static void lvgl_flush_cb(lv_display_t* display, const lv_area_t* area, uint8_t* px_map)
{
    const uint32_t start_us = static_cast<uint32_t>(k_cyc_to_us_floor64(k_cycle_get_32()));
    const int32_t x0 = area->x1;
    const int32_t y0 = area->y1;
    const int32_t x1 = area->x2;
    const int32_t y1 = area->y2;

    if ((x1 < 0) || (y1 < 0) || (x0 >= k_lcd_width) || (y0 >= k_lcd_height)) {
        lv_display_flush_ready(display);
        return;
    }

    const uint16_t clipped_x0 = static_cast<uint16_t>(x0 < 0 ? 0 : x0);
    const uint16_t clipped_y0 = static_cast<uint16_t>(y0 < 0 ? 0 : y0);
    const uint16_t clipped_x1 = static_cast<uint16_t>(x1 >= k_lcd_width ? k_lcd_width - 1 : x1);
    const uint16_t clipped_y1 = static_cast<uint16_t>(y1 >= k_lcd_height ? k_lcd_height - 1 : y1);
    const uint16_t width      = static_cast<uint16_t>(clipped_x1 - clipped_x0 + 1U);
    const uint16_t height     = static_cast<uint16_t>(clipped_y1 - clipped_y0 + 1U);
    const uint16_t area_width = static_cast<uint16_t>(x1 - x0 + 1);
    const auto* src           = reinterpret_cast<const uint16_t*>(px_map);

    for (uint16_t row = 0U; row < height; ++row) {
        const int32_t src_y = static_cast<int32_t>(clipped_y0) + row - y0;
        const int32_t src_x = static_cast<int32_t>(clipped_x0) - x0;
        const auto* src_row = &src[static_cast<size_t>(src_y) * area_width + static_cast<size_t>(src_x)];
        for (uint16_t col = 0U; col < width; ++col) {
            g_flush_line_buffer[col] = src_row[col];
        }

        (void)ili9481_support_blit_rgb565(clipped_x0, static_cast<uint16_t>(clipped_y0 + row), width, 1U,
                                          g_flush_line_buffer);
    }

    if (lv_display_flush_is_last(display)) {
        ++g_frame_count;
    }

    const uint32_t elapsed_us = static_cast<uint32_t>(k_cyc_to_us_floor64(k_cycle_get_32())) - start_us;
    ++g_flush_stats.flush_count;
    g_flush_stats.pixel_count += static_cast<uint32_t>(width) * static_cast<uint32_t>(height);
    g_flush_stats.total_us += elapsed_us;
    if (elapsed_us > g_flush_stats.max_us) {
        g_flush_stats.max_us = elapsed_us;
    }

    lv_display_flush_ready(display);
}

static void lvgl_create_perf_hud()
{
    g_perf_hud_label = lv_label_create(lv_layer_top());
    lv_obj_align(g_perf_hud_label, LV_ALIGN_TOP_RIGHT, -4, 4);
    lv_obj_set_style_bg_color(g_perf_hud_label, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_perf_hud_label, LV_OPA_70, 0);
    lv_obj_set_style_text_color(g_perf_hud_label, lv_color_white(), 0);
    lv_obj_set_style_pad_all(g_perf_hud_label, 4, 0);
    lv_label_set_text(g_perf_hud_label, "FPS --\nflush --");
}

static void lvgl_update_perf_hud()
{
    if (g_perf_hud_label == nullptr) {
        return;
    }

    const uint32_t now_ms = static_cast<uint32_t>(k_uptime_get_32());
    const uint32_t elapsed_ms = now_ms - g_last_hud_ms;
    if (elapsed_ms < k_perf_hud_period_ms) {
        return;
    }

    lv_mem_monitor_t mem{};
    lv_mem_monitor(&mem);

    const uint32_t fps = (g_frame_count * 1000U) / elapsed_ms;
    const uint32_t avg_flush_us = g_flush_stats.flush_count == 0U ? 0U : g_flush_stats.total_us / g_flush_stats.flush_count;
    const uint32_t kpixels_per_s = elapsed_ms == 0U ? 0U : g_flush_stats.pixel_count / elapsed_ms;

    lv_label_set_text_fmt(g_perf_hud_label,
                          "FPS %u\nflush %u avg %uus max %uus\npix %u kpx/s\nheap %u free %u%% frag",
                          static_cast<unsigned int>(fps), static_cast<unsigned int>(g_flush_stats.flush_count),
                          static_cast<unsigned int>(avg_flush_us), static_cast<unsigned int>(g_flush_stats.max_us),
                          static_cast<unsigned int>(kpixels_per_s), static_cast<unsigned int>(mem.free_size),
                          static_cast<unsigned int>(mem.frag_pct));

    g_frame_count = 0U;
    g_flush_stats = LvglFlushStats{};
    g_last_hud_ms = now_ms;
}

static Result<void> lvgl_display_init()
{
    if (!ili9481_support_ready()) {
        const int ret = ili9481_support_init();
        if (ret != 0) {
            LOG_ERR("ILI9481 init failed for LVGL: %d", ret);
            return ErrorCode::HardwareFault;
        }
    }

    lv_init();
    lv_tick_set_cb(lvgl_tick_get_cb);
    lv_delay_set_cb(lvgl_delay_cb);

    g_display = lv_display_create(k_lcd_width, k_lcd_height);
    if (g_display == nullptr) {
        return ErrorCode::NoMemory;
    }

    lv_display_set_color_format(g_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(g_display, lvgl_flush_cb);
    lv_display_set_buffers(g_display, g_draw_buffer_0, g_draw_buffer_1, k_lvgl_draw_buffer_bytes,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_default(g_display);
    lvgl_create_perf_hud();
    g_last_hud_ms = static_cast<uint32_t>(k_uptime_get_32());
    return ErrorCode::Ok;
}

static void lvgl_thread_entry(void*, void*, void*)
{
    auto init_result = lvgl_display_init();
    if (init_result.is_error()) {
        LOG_ERR("LVGL init failed: %d", static_cast<int>(init_result.error()));
        return;
    }

    atomic_set(&g_ready, 1);
    LOG_INF("LVGL ready: %dx%d draw lines=%u", k_lcd_width, k_lcd_height,
            static_cast<unsigned int>(k_lvgl_draw_lines));

    while (true) {
        if (atomic_cas(&g_stress_requested, 1, 0)) {
            lv_obj_clean(lv_screen_active());
            lv_demo_stress();
            atomic_set(&g_stress_running, 1);
            LOG_INF("LVGL stress test started");
        }

        const uint32_t wait_ms = lv_timer_handler();
        lvgl_update_perf_hud();

        if (atomic_get(&g_stress_running) && lv_demo_stress_finished()) {
            atomic_set(&g_stress_running, 0);
            LOG_INF("LVGL stress test round finished");
        }

        const uint32_t delay_ms = wait_ms == LV_NO_TIMER_READY ? k_lvgl_handler_period_ms : wait_ms;
        k_msleep(delay_ms > k_lvgl_handler_period_ms ? k_lvgl_handler_period_ms : delay_ms);
    }
}

Result<void> lvgl_port_init()
{
    if (g_lvgl_thread_id != nullptr) {
        return ErrorCode::Ok;
    }

    g_lvgl_thread_id = k_thread_create(&g_lvgl_thread, g_lvgl_thread_stack, K_THREAD_STACK_SIZEOF(g_lvgl_thread_stack),
                                      lvgl_thread_entry, nullptr, nullptr, nullptr, 7, 0, K_NO_WAIT);
    if (g_lvgl_thread_id == nullptr) {
        return ErrorCode::InvalidState;
    }

    k_thread_name_set(g_lvgl_thread_id, "lvgl");
    return ErrorCode::Ok;
}

Result<void> lvgl_port_start_stress_test()
{
    if (!atomic_get(&g_ready)) {
        return ErrorCode::InvalidState;
    }

    atomic_set(&g_stress_requested, 1);
    return ErrorCode::Ok;
}

bool lvgl_port_ready()
{
    return atomic_get(&g_ready) != 0;
}

bool lvgl_port_stress_test_requested()
{
    return (atomic_get(&g_stress_requested) != 0) || (atomic_get(&g_stress_running) != 0);
}

} // namespace omnigen
