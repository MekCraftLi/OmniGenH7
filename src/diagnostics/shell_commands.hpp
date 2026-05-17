/**
 *******************************************************************************
 * @file    shell_commands.hpp
 * @brief   Shell commands for testing signal engine
 *******************************************************************************
 * @attention
 *
 * Shell commands for development and testing:
 * - signal start/stop/pause/resume
 * - signal freq <hz>
 * - signal amp <mv>
 * - signal waveform <sine|square|triangle|sawtooth>
 * - signal status
 *
 *******************************************************************************
 * @note
 *
 * Commands are registered with Zephyr shell subsystem.
 * Use for development testing without UI.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "ports/command_bus_port.hpp"
#include "ports/request_bus_port.hpp"

namespace omnigen {

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief Register shell commands for signal engine.
 *
 * Call once during initialization.
 */
void register_signal_shell_commands(CommandBusPort& command_bus, RequestBusPort& request_bus);

} // namespace omnigen
