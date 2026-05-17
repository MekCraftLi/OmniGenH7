/**
 *******************************************************************************
 * @file    signal_command.cpp
 * @brief   Signal command factory implementations
 *******************************************************************************
 * @attention
 *
 * Factory helper functions for creating SignalCommand instances.
 *
 *******************************************************************************
 * @note
 *
 * Using factory functions ensures consistent command construction
 * and makes the code more readable at call sites.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "domain/signal_command.hpp"

namespace omnigen {

/*-------- 2. variables ----------------------------------------------------------------------------------------------*/

static uint32_t g_command_sequence = 0;

/*-------- 3. implementation -----------------------------------------------------------------------------------------*/

SignalCommand SignalCommand::make_start(CommandSource src)
{
    SignalCommand cmd{};
    cmd.kind = SignalCommandKind::Start;
    cmd.source = src;
    cmd.sequence = ++g_command_sequence;
    return cmd;
}

SignalCommand SignalCommand::make_stop(CommandSource src)
{
    SignalCommand cmd{};
    cmd.kind = SignalCommandKind::Stop;
    cmd.source = src;
    cmd.sequence = ++g_command_sequence;
    return cmd;
}

SignalCommand SignalCommand::make_pause(CommandSource src)
{
    SignalCommand cmd{};
    cmd.kind = SignalCommandKind::Pause;
    cmd.source = src;
    cmd.sequence = ++g_command_sequence;
    return cmd;
}

SignalCommand SignalCommand::make_resume(CommandSource src)
{
    SignalCommand cmd{};
    cmd.kind = SignalCommandKind::Resume;
    cmd.source = src;
    cmd.sequence = ++g_command_sequence;
    return cmd;
}

SignalCommand SignalCommand::make_set_frequency(CommandSource src, FrequencyHz freq)
{
    SignalCommand cmd{};
    cmd.kind = SignalCommandKind::SetFrequency;
    cmd.source = src;
    cmd.sequence = ++g_command_sequence;
    cmd.frequency = freq;
    return cmd;
}

SignalCommand SignalCommand::make_set_amplitude(CommandSource src, VoltageMv amp)
{
    SignalCommand cmd{};
    cmd.kind = SignalCommandKind::SetAmplitude;
    cmd.source = src;
    cmd.sequence = ++g_command_sequence;
    cmd.amplitude = amp;
    return cmd;
}

SignalCommand SignalCommand::make_set_waveform(CommandSource src, WaveformKind kind)
{
    SignalCommand cmd{};
    cmd.kind = SignalCommandKind::SetWaveform;
    cmd.source = src;
    cmd.sequence = ++g_command_sequence;
    cmd.waveform = kind;
    return cmd;
}

SignalCommand SignalCommand::make_clear_fault(CommandSource src)
{
    SignalCommand cmd{};
    cmd.kind = SignalCommandKind::ClearFault;
    cmd.source = src;
    cmd.sequence = ++g_command_sequence;
    return cmd;
}

} // namespace omnigen