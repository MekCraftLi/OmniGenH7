/**
 *******************************************************************************
 * @file    composition_root.hpp
 * @brief   System composition root for dependency injection
 *******************************************************************************
 * @attention
 *
 * The composition root is responsible for:
 * - Creating all service objects
 * - Injecting dependencies (port implementations into services)
 * - Initializing hardware and platform layer
 * - Starting threads (in later phases)
 *
 *******************************************************************************
 * @note
 *
 * This file should be the only place where concrete implementations
 * are instantiated and wired together. Other modules should not
 * create global services directly.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/05
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include <stdbool.h>

namespace omnigen {

/* ------- function declare ------------------------------------------------------------------------------------------*/

/**
 * @brief Initialize all system components.
 *
 * Called once at startup before entering main loop.
 * Creates services, injects dependencies, and initializes hardware.
 */
void init_system();

/**
 * @brief Check if system initialization completed successfully.
 */
bool is_system_ready();

} // namespace omnigen