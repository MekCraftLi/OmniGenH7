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
