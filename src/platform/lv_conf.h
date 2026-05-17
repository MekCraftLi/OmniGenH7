/**
 *******************************************************************************
 * @file    lv_conf.h
 * @brief   LVGL configuration for OmniGen H7
 *******************************************************************************
 * @attention
 *
 * This configuration is used by the LVGL submodule build and keeps LVGL tuned for
 * the STM32H723 SRAM budget and RGB565 ILI9481 panel.
 *
 *******************************************************************************
 * @note
 *
 * LVGL uses a partial draw buffer; the project does not allocate a full-screen
 * framebuffer.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */
#pragma once
#ifndef LV_CONF_H
#define LV_CONF_H

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_MEM_SIZE          (96U * 1024U)

#define LV_COLOR_DEPTH 16
#define LV_USE_OS     LV_OS_NONE

#define LV_USE_LOG 1
#if LV_USE_LOG
#define LV_LOG_LEVEL      LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF     0
#define LV_LOG_USE_TIMESTAMP 1
#endif

#define LV_USE_ASSERT_NULL     1
#define LV_USE_ASSERT_MALLOC   1
#define LV_USE_ASSERT_STYLE    0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ      0

#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR  0

#define LV_USE_DEMO_STRESS 1
#define LV_BUILD_EXAMPLES  0
#define LV_BUILD_DEMOS     1

#endif /* LV_CONF_H */
