/**
 *******************************************************************************
 * @file    composition_root.cpp
 * @brief   System composition root implementation
 *******************************************************************************
 * @attention
 *
 * This file implements the system initialization and dependency injection.
 * Currently a skeleton - services will be added in later phases.
 *
 *******************************************************************************
 * @note
 *
 * Phase 1: Basic initialization and logging
 * Phase 2: Create port implementations
 * Phase 3: Create services and inject dependencies
 * Phase 4: Start threads
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/05
 * @version 1.0
 *******************************************************************************
 */

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "composition_root.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(composition_root, CONFIG_LOG_DEFAULT_LEVEL);

/* ------- variables -------------------------------------------------------------------------------------------------*/

static bool g_system_ready = false;

/* ------- function implement ----------------------------------------------------------------------------------------*/

void omnigen::init_system()
{
    LOG_INF("OmniGen H7 initializing...");

    /* Phase 1: Basic initialization (current) */
    /* TODO: Phase 2 - Create port implementations */
    /* TODO: Phase 3 - Create services and inject dependencies */
    /* TODO: Phase 4 - Start threads */

    g_system_ready = true;
    LOG_INF("OmniGen H7 ready");
}

bool omnigen::is_system_ready()
{
    return g_system_ready;
}
