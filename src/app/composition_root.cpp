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

#include "platform/mock_wave_sink.hpp"
#include "platform/zephyr_storage.hpp"
#include "services/signal_engine.hpp"
#include "diagnostics/shell_commands.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <new>

LOG_MODULE_REGISTER(composition_root, CONFIG_LOG_DEFAULT_LEVEL);

namespace omnigen {

/* ------- variables -------------------------------------------------------------------------------------------------*/

static bool g_system_ready = false;

/* Port implementations (static storage) */
static MockWaveSink g_wave_sink;
static ZephyrStorage g_storage;

/* Services (static storage) */
static SignalEngine g_signal_engine(g_wave_sink);

/* ------- function implement ----------------------------------------------------------------------------------------*/

void init_system()
{
    auto storage_ret = g_storage.mount();
    if (storage_ret.is_error()) {
        LOG_WRN("External storage init failed: %d", static_cast<int>(storage_ret.error()));
    }

    LOG_INF("OmniGen H7 initializing...");

    /* Phase 1: Create port implementations */
    LOG_INF("Creating port implementations...");

    /* Phase 2: Services are already constructed statically */

    /* Phase 3: Register shell commands */
    LOG_INF("Registering shell commands...");
    register_signal_shell_commands(g_signal_engine);

    /* Phase 4: Mark ready */
    g_system_ready = true;
    LOG_INF("OmniGen H7 ready");
    LOG_INF("Use 'signal status' to check state");
    LOG_INF("Use 'signal start' to begin output");
}

bool is_system_ready()
{
    return g_system_ready;
}

} // namespace omnigen
