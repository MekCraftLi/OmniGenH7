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

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "services/signal_engine.hpp"

namespace omnigen {

/* ------- function declare ------------------------------------------------------------------------------------------*/

/**
 * @brief Register shell commands for signal engine.
 *
 * Call once during initialization.
 */
void register_signal_shell_commands(SignalEngine& engine);

} // namespace omnigen
