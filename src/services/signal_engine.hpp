/**
 *******************************************************************************
 * @file    signal_engine.hpp
 * @brief   Signal engine state machine for signal generation control
 *******************************************************************************
 * @attention
 *
 * SignalEngine is the main business state machine that handles:
 * - User commands (start/stop/pause/resume)
 * - Parameter changes (frequency/amplitude/waveform)
 * - State transitions and validation
 * - Coordination with OutputScheduler and WaveformRepository
 *
 *******************************************************************************
 * @note
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
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "domain/signal_command.hpp"
#include "domain/signal_profile.hpp"
#include "domain/signal_state.hpp"
#include "domain/signal_snapshot.hpp"
#include "domain/signal_validation.hpp"
#include "ports/wave_sink_port.hpp"

namespace omnigen {

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief Signal engine state machine.
 *
 * Handles business logic for signal generation.
 * Coordinates with WaveSinkPort for output control.
 */
class SignalEngine {
public:
    /**
     * @brief Construct signal engine with wave sink.
     */
    explicit SignalEngine(WaveSinkPort& sink);

    /**
     * @brief Handle a command.
     * @param command The command to process.
     * @return Result<void> Ok if command handled, error if invalid.
     */
    Result<void> handle_command(const SignalCommand& command);

    /**
     * @brief Get current state snapshot.
     */
    SignalEngineSnapshot snapshot() const;

    /**
     * @brief Get current state.
     */
    SignalEngineState state() const { return state_; }

    /**
     * @brief Get active profile.
     */
    const SignalProfile& active_profile() const { return active_profile_; }

    /**
     * @brief Check if output is active.
     */
    bool is_output_active() const;

private:
    /* ------- private methods ---------------------------------------------------------------------------------------*/

    Result<void> handle_start();
    Result<void> handle_stop();
    Result<void> handle_pause();
    Result<void> handle_resume();
    Result<void> handle_set_frequency(FrequencyHz freq);
    Result<void> handle_set_amplitude(VoltageMv amp);
    Result<void> handle_set_waveform(WaveformKind kind);
    Result<void> handle_clear_fault();

    bool can_start() const;
    void transition_to(SignalEngineState new_state);

    /* ------- member variables --------------------------------------------------------------------------------------*/

    WaveSinkPort& sink_;
    SignalEngineState state_;
    SignalProfile active_profile_;
    SignalProfile pending_profile_;
    SignalLimits limits_;
    WaveformId selected_waveform_;
    ErrorCode last_fault_;
    uint32_t command_count_;
};

} // namespace omnigen
