/**
 *******************************************************************************
 * @file    composition_root.cpp
 * @brief   System composition root implementation
 *******************************************************************************
 * @attention
 *
 * This file implements the system initialization and dependency injection.
 * Creates all services and wires dependencies together.
 *
 *******************************************************************************
 * @note
 *
 * Composition root is the only place where:
 * - Concrete implementations are instantiated
 * - Dependencies are injected
 * - Global services are created
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/05
 * @version 1.0
 *******************************************************************************
 */

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "composition_root.hpp"

#include "diagnostics/shell_commands.hpp"
#include "platform/zephyr/dac_control_manager.hpp"
#include "platform/zephyr/application_base.h"
#include "platform/zephyr/led_blink_manager.hpp"
#include "services/signal_engine.hpp"

#include <new>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(composition_root, CONFIG_LOG_DEFAULT_LEVEL);

namespace omnigen {

/* ------- variables -------------------------------------------------------------------------------------------------*/

static bool g_system_ready = false;

/* Services (static storage) */
static SignalEngine g_signal_engine(DacControlManager::instance());

/* ------- function implement ----------------------------------------------------------------------------------------*/

void init_system() {
    LOG_INF("OmniGen H7 initializing...");

    /* Phase 1: Create port implementations */
    LOG_INF("Creating port implementations...");

    /* Phase 2: Services and application managers are already constructed statically */
    (void)LedBlinkManager::instance();
    (void)DacControlManager::instance();

    /* Phase 3: Register shell commands */
    LOG_INF("Registering shell commands...");
    register_signal_shell_commands(g_signal_engine);

    /* Phase 4: Start hardware manager threads */
    LOG_INF("Starting hardware manager threads...");
    StaticAppBase::startApplications();

    /* Phase 5: Mark ready */
    g_system_ready = true;
    LOG_INF("OmniGen H7 ready");
    LOG_INF("Use 'signal status' to check state");
    LOG_INF("Use 'signal start' to begin output");
}

bool is_system_ready() { return g_system_ready; }

} // namespace omnigen
