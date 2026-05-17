/**
 *******************************************************************************
 * @file    signal_engine.cpp
 * @brief   Signal engine state machine implementation
 *******************************************************************************
 * @attention
 *
 * Implements the main business state machine for signal generation.
 * All state transitions are validated before execution.
 *
 *******************************************************************************
 * @note
 *
 * The engine does not directly access hardware. All output operations
 * go through WaveSinkPort interface, allowing for testing with mocks.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "services/signal_engine.hpp"

namespace omnigen {

/*-------- 3. implementation -----------------------------------------------------------------------------------------*/

SignalEngine::SignalEngine(WaveSinkPort& sink)
    : sink_(sink)
    , state_(SignalEngineState::Idle)
    , active_profile_(get_default_signal_profile())
    , pending_profile_(get_default_signal_profile())
    , limits_(get_default_signal_limits())
    , last_fault_(ErrorCode::Ok)
    , command_count_(0)
{
}

Result<void> SignalEngine::handle_command(const SignalCommand& command)
{
    command_count_++;

    switch (command.kind) {
        case SignalCommandKind::Start:
            return handle_start();

        case SignalCommandKind::Stop:
            return handle_stop();

        case SignalCommandKind::Pause:
            return handle_pause();

        case SignalCommandKind::Resume:
            return handle_resume();

        case SignalCommandKind::SetFrequency:
            return handle_set_frequency(command.frequency);

        case SignalCommandKind::SetAmplitude:
            return handle_set_amplitude(command.amplitude);

        case SignalCommandKind::SetWaveform:
            return handle_set_waveform(command.waveform);

        case SignalCommandKind::ClearFault:
            return handle_clear_fault();

        default:
            return ErrorCode::InvalidArgument;
    }
}

SignalEngineSnapshot SignalEngine::snapshot() const
{
    return SignalEngineSnapshot{
        .state = state_,
        .active_profile = active_profile_,
        .selected_waveform = selected_waveform_,
        .last_fault = last_fault_,
        .command_count = command_count_,
    };
}

bool SignalEngine::is_output_active() const
{
    return state_ == SignalEngineState::Running ||
           state_ == SignalEngineState::Arming;
}

/*-------- 4. private methods ----------------------------------------------------------------------------------------*/

Result<void> SignalEngine::handle_start()
{
    if (state_ != SignalEngineState::Idle) {
        return ErrorCode::InvalidState;
    }

    if (!can_start()) {
        return ErrorCode::InvalidState;
    }

    /* Validate profile before starting */
    Result<void> validation = validate_signal_profile(active_profile_, limits_);
    if (validation.is_error()) {
        return validation.error();
    }

    /* Configure output */
    Result<void> config_result = sink_.configure(active_profile_);
    if (config_result.is_error()) {
        last_fault_ = config_result.error();
        transition_to(SignalEngineState::Fault);
        return config_result.error();
    }

    /* Start output */
    Result<void> start_result = sink_.start();
    if (start_result.is_error()) {
        last_fault_ = start_result.error();
        transition_to(SignalEngineState::Fault);
        return start_result.error();
    }

    transition_to(SignalEngineState::Running);
    return ErrorCode::Ok;
}

Result<void> SignalEngine::handle_stop()
{
    if (state_ != SignalEngineState::Running &&
        state_ != SignalEngineState::Paused) {
        return ErrorCode::InvalidState;
    }

    Result<void> stop_result = sink_.stop();
    if (stop_result.is_error()) {
        last_fault_ = stop_result.error();
        transition_to(SignalEngineState::Fault);
        return stop_result.error();
    }

    transition_to(SignalEngineState::Idle);
    return ErrorCode::Ok;
}

Result<void> SignalEngine::handle_pause()
{
    if (state_ != SignalEngineState::Running) {
        return ErrorCode::InvalidState;
    }

    Result<void> stop_result = sink_.stop();
    if (stop_result.is_error()) {
        last_fault_ = stop_result.error();
        transition_to(SignalEngineState::Fault);
        return stop_result.error();
    }

    transition_to(SignalEngineState::Paused);
    return ErrorCode::Ok;
}

Result<void> SignalEngine::handle_resume()
{
    if (state_ != SignalEngineState::Paused) {
        return ErrorCode::InvalidState;
    }

    Result<void> start_result = sink_.start();
    if (start_result.is_error()) {
        last_fault_ = start_result.error();
        transition_to(SignalEngineState::Fault);
        return start_result.error();
    }

    transition_to(SignalEngineState::Running);
    return ErrorCode::Ok;
}

Result<void> SignalEngine::handle_set_frequency(FrequencyHz freq)
{
    if (state_ == SignalEngineState::Running) {
        return ErrorCode::InvalidState;
    }

    SignalProfile new_profile = active_profile_;
    new_profile.frequency = freq;

    Result<void> validation = validate_signal_profile(new_profile, limits_);
    if (validation.is_error()) {
        return validation.error();
    }

    active_profile_ = new_profile;
    return ErrorCode::Ok;
}

Result<void> SignalEngine::handle_set_amplitude(VoltageMv amp)
{
    if (state_ == SignalEngineState::Running) {
        return ErrorCode::InvalidState;
    }

    SignalProfile new_profile = active_profile_;
    new_profile.amplitude = amp;

    Result<void> validation = validate_signal_profile(new_profile, limits_);
    if (validation.is_error()) {
        return validation.error();
    }

    active_profile_ = new_profile;
    return ErrorCode::Ok;
}

Result<void> SignalEngine::handle_set_waveform(WaveformKind kind)
{
    if (state_ == SignalEngineState::Running) {
        return ErrorCode::InvalidState;
    }

    active_profile_.kind = kind;
    return ErrorCode::Ok;
}

Result<void> SignalEngine::handle_clear_fault()
{
    if (state_ != SignalEngineState::Fault) {
        return ErrorCode::InvalidState;
    }

    last_fault_ = ErrorCode::Ok;
    transition_to(SignalEngineState::Idle);
    return ErrorCode::Ok;
}

bool SignalEngine::can_start() const
{
    return active_profile_.output_enabled ||
           active_profile_.kind != WaveformKind::None;
}

void SignalEngine::transition_to(SignalEngineState new_state)
{
    state_ = new_state;
}

} // namespace omnigen
