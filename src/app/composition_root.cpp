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

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "composition_root.hpp"

#include "diagnostics/shell_commands.hpp"
#include "platform/lvgl_port.hpp"
#include "platform/zephyr_display.hpp"
#include "platform/zephyr_filter_switch.hpp"
#include "platform/mpu_manager.hpp"
#include "platform/zephyr_storage.hpp"
#include "platform/zephyr_wave_sink.hpp"
#include "services/direct_command_bus.hpp"
#include "services/direct_request_bus.hpp"
#include "services/signal_engine.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <new>

LOG_MODULE_REGISTER(composition_root, CONFIG_LOG_DEFAULT_LEVEL);

namespace omnigen {

/*-------- 2. data structures ----------------------------------------------------------------------------------------*/

/**
 * @brief 系统实例上下文结构体。
 *
 * 该结构集中拥有平台适配器、领域服务和应用总线实例，是组合根中的依赖注入
 * 容器。成员构造顺序即依赖顺序，避免在其他模块中创建全局单例。
 */
struct SystemContext {
    ZephyrWaveSink wave_sink;
    ZephyrStorage storage;
    ZephyrDisplay display;
    ZephyrFilterSwitch filter_switch;
    SignalEngine signal_engine;
    DirectCommandBus command_bus;
    DirectRequestBus request_bus;

    SystemContext(bool& system_ready)
        : wave_sink()
        , storage()
        , display()
        , filter_switch()
        , signal_engine(wave_sink)
        , command_bus(signal_engine, filter_switch)
        , request_bus(signal_engine, filter_switch, storage, display, system_ready)
    {
    }
};

/*-------- 3. variables ----------------------------------------------------------------------------------------------*/

static bool g_system_ready = false;
static SystemContext g_system(g_system_ready);

/*-------- 4. implementation -----------------------------------------------------------------------------------------*/

void init_system()
{
    MpuManager::init();

    auto storage_ret = g_system.storage.initialize();
    if (storage_ret.is_error()) {
        LOG_WRN("External storage init failed: %d", static_cast<int>(storage_ret.error()));
    }

    auto display_ret = g_system.display.initialize();
    if (display_ret.is_error()) {
        LOG_WRN("Display init failed: %d", static_cast<int>(display_ret.error()));
    } else {
        (void)g_system.display.clear(0x0000U);
        auto lvgl_ret = lvgl_port_init();
        if (lvgl_ret.is_error()) {
            LOG_WRN("LVGL init failed: %d", static_cast<int>(lvgl_ret.error()));
        }
    }

    auto filter_ret = g_system.filter_switch.initialize();
    if (filter_ret.is_error()) {
        LOG_WRN("Filter switch init failed: %d", static_cast<int>(filter_ret.error()));
    }

    LOG_INF("OmniGen H7 initializing...");

    /* Phase 1: Create port implementations */
    LOG_INF("Creating port implementations...");

    /* Phase 2: Services are already constructed statically */

    /* Phase 3: Register shell commands */
    LOG_INF("Registering shell commands...");
    register_signal_shell_commands(g_system.command_bus, g_system.request_bus);

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
