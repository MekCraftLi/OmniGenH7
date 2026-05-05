/**
 *******************************************************************************
 * @file    signal_command.hpp
 * @brief   Signal command definitions for control flow
 *******************************************************************************
 * @attention
 *
 * Commands represent "what to do" requests from user input or host protocol.
 * Commands are processed by SignalEngine state machine.
 *
 *******************************************************************************
 * @note
 *
 * Command flow:
 * Button/Encoder/Host -> InputEvent -> InputRouter -> SignalCommand -> SignalEngine
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "signal_profile.hpp"
#include "base/result.hpp"

#include <cstdint>

namespace omnigen {

/* ------- enum ------------------------------------------------------------------------------------------------------*/

/**
 * @brief Signal command types.
 */
enum class SignalCommandKind : uint8_t {
    None,
    Start,
    Stop,
    Pause,
    Resume,
    SetFrequency,
    SetAmplitude,
    SetOffset,
    SetWaveform,
    SetDuty,
    LoadPreset,
    SavePreset,
    ClearFault,
};

/**
 * @brief Command source identifier.
 */
enum class CommandSource : uint8_t {
    None,
    Button,
    Encoder,
    HostUart,
    Shell,
    Internal,
};

/* ------- class prototypes ------------------------------------------------------------------------------------------*/

/**
 * @brief Signal command with parameters.
 */
struct SignalCommand {
    SignalCommandKind kind;
    CommandSource source;
    uint32_t sequence;

    /* Command parameters (union would save space, but struct is simpler) */
    FrequencyHz frequency;
    VoltageMv amplitude;
    VoltageMv offset;
    WaveformKind waveform;
    DutyPermille duty;
    uint32_t preset_id;

    /* Factory helpers */
    static SignalCommand make_start(CommandSource src);
    static SignalCommand make_stop(CommandSource src);
    static SignalCommand make_pause(CommandSource src);
    static SignalCommand make_resume(CommandSource src);
    static SignalCommand make_set_frequency(CommandSource src, FrequencyHz freq);
    static SignalCommand make_set_amplitude(CommandSource src, VoltageMv amp);
    static SignalCommand make_set_waveform(CommandSource src, WaveformKind kind);
    static SignalCommand make_clear_fault(CommandSource src);
};

} // namespace omnigen