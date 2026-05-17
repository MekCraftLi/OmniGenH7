/**
 *******************************************************************************
 * @file    direct_command_bus.hpp
 * @brief   Direct in-process implementation of CommandBusPort
 *******************************************************************************
 * @attention
 *
 * DirectCommandBus dispatches commands synchronously to service and platform
 * objects in the current thread.
 *
 *******************************************************************************
 * @note
 *
 * This implementation is suitable for shell and bring-up flows. It can be
 * replaced later by a queued implementation without changing command producers.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "ports/command_bus_port.hpp"
#include "ports/filter_switch_port.hpp"
#include "services/signal_engine.hpp"

namespace omnigen {

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 进程内同步命令总线实现类。
 *
 * 将 `AppCommand` 按类型直接分发给当前组合根中的服务或平台端口。该实现不排队、
 * 不缓存命令，适合启动调试、Shell 控制和早期硬件 bring-up。
 */
class DirectCommandBus : public CommandBusPort {
public:
    DirectCommandBus(SignalEngine& signal_engine, FilterSwitchPort& filter_switch);
    ~DirectCommandBus() override = default;

    Result<void> submit(const AppCommand& command) override;

private:
    SignalEngine& signal_engine_;
    FilterSwitchPort& filter_switch_;
};

} // namespace omnigen
