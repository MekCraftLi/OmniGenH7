/**
 *******************************************************************************
 * @file    command_bus_port.cpp
 * @brief   Application command factory implementations
 *******************************************************************************
 * @attention
 *
 * Factory helpers keep application command construction consistent across shell,
 * input, and host protocol adapters.
 *
 *******************************************************************************
 * @note
 *
 * Command sequence counters are local to each command family.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "ports/command_bus_port.hpp"

namespace omnigen {

/*-------- 2. variables ----------------------------------------------------------------------------------------------*/

static uint32_t g_filter_command_sequence = 0U;

/*-------- 3. implementation -----------------------------------------------------------------------------------------*/

FilterCommand FilterCommand::make_set_mode(CommandSource source, FilterMode mode)
{
    FilterCommand command{};
    command.kind = FilterCommandKind::SetMode;
    command.source = source;
    command.sequence = ++g_filter_command_sequence;
    command.mode = mode;
    return command;
}

FilterCommand FilterCommand::make_set_mute(CommandSource source, bool enabled)
{
    FilterCommand command{};
    command.kind = FilterCommandKind::SetMute;
    command.source = source;
    command.sequence = ++g_filter_command_sequence;
    command.mute = enabled;
    return command;
}

AppCommand AppCommand::make_signal(const SignalCommand& command)
{
    AppCommand app_command{};
    app_command.kind = AppCommandKind::Signal;
    app_command.signal = command;
    return app_command;
}

AppCommand AppCommand::make_filter(const FilterCommand& command)
{
    AppCommand app_command{};
    app_command.kind = AppCommandKind::Filter;
    app_command.filter = command;
    return app_command;
}

} // namespace omnigen
