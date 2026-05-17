/**
 *******************************************************************************
 * @file    signal_state.hpp
 * @brief   Signal engine and output state enumerations
 *******************************************************************************
 * @attention
 *
 * State enumerations for signal engine and output scheduler.
 * These states drive the main state machines in the system.
 *
 *******************************************************************************
 * @note
 *
 * SignalEngineState: Main business state machine (Idle -> Running -> etc.)
 * OutputState: Output hardware state machine (Configured -> Streaming -> etc.)
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/05
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cstdint>

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

/**
 * @brief Signal engine business state.
 *
 * State transitions:
 * - Idle -> Editing (user edits parameters)
 * - Idle -> Arming (user presses start)
 * - Arming -> Running (output ready)
 * - Running -> Paused (user pauses)
 * - Paused -> Running (user resumes)
 * - Running -> Stopping (user stops)
 * - Stopping -> Idle (output stopped)
 * - Any -> Fault (error detected)
 */
enum class SignalEngineState : uint8_t {
    Idle,
    Editing,
    Arming,
    Running,
    Paused,
    Stopping,
    Fault,
};

/**
 * @brief Output scheduler hardware state.
 *
 * State transitions:
 * - Unconfigured -> Configured (profile set)
 * - Configured -> Priming (buffer filling)
 * - Priming -> Streaming (DMA running)
 * - Streaming -> Draining (stop requested)
 * - Draining -> Stopped (DMA complete)
 * - Streaming -> Underrun (buffer empty)
 * - Any -> Fault (DMA/DAC error)
 */
enum class OutputState : uint8_t {
    Unconfigured,
    Configured,
    Priming,
    Streaming,
    Draining,
    Stopped,
    Underrun,
    Fault,
};

} // namespace omnigen