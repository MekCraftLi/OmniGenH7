/**
 *******************************************************************************
 * @file    lvgl_port.hpp
 * @brief   LVGL platform adapter for OmniGen H7
 *******************************************************************************
 * @attention
 *
 * This module owns the LVGL display registration and GUI worker thread.
 *
 *******************************************************************************
 * @note
 *
 * Shell commands enqueue GUI work through this adapter instead of calling LVGL
 * directly from shell context.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */
#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "base/result.hpp"

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

namespace omnigen {

Result<void> lvgl_port_init();
Result<void> lvgl_port_start_stress_test();
bool lvgl_port_ready();
bool lvgl_port_stress_test_requested();

} // namespace omnigen
