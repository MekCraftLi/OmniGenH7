/**
 *******************************************************************************
 * @file    direct_command_bus.cpp
 * @brief   Direct in-process implementation of CommandBusPort
 *******************************************************************************
 * @attention
 *
 * This file dispatches typed application commands to the current concrete service
 * graph without queueing or thread handoff.
 *
 *******************************************************************************
 * @note
 *
 * Submit returns the target handler result directly.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "services/direct_command_bus.hpp"

namespace omnigen {

/*-------- 3. implementation -----------------------------------------------------------------------------------------*/

DirectCommandBus::DirectCommandBus(SignalEngine& signal_engine, FilterSwitchPort& filter_switch)
    : signal_engine_(signal_engine)
    , filter_switch_(filter_switch)
{
}

Result<void> DirectCommandBus::submit(const AppCommand& command)
{
    switch (command.kind) {
        case AppCommandKind::Signal:
            return signal_engine_.handle_command(command.signal);

        case AppCommandKind::Filter:
            switch (command.filter.kind) {
                case FilterCommandKind::SetMode:
                    return filter_switch_.set_mode(command.filter.mode);
                case FilterCommandKind::SetMute:
                    return filter_switch_.set_mute(command.filter.mute);
                default:
                    return ErrorCode::InvalidArgument;
            }

        default:
            return ErrorCode::InvalidArgument;
    }
}

} // namespace omnigen
