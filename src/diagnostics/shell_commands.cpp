/**
 *******************************************************************************
 * @file    shell_commands.cpp
 * @brief   Shell commands implementation for signal engine testing
 *******************************************************************************
 * @attention
 *
 * Implements Zephyr shell commands for signal engine control.
 * Commands are under "signal" subcommand group.
 *
 *******************************************************************************
 * @note
 *
 * Usage examples:
 * - signal start
 * - signal stop
 * - signal freq 1000
 * - signal amp 1500
 * - signal waveform sine
 * - signal status
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "diagnostics/shell_commands.hpp"

#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(signal_shell, CONFIG_LOG_DEFAULT_LEVEL);

namespace omnigen {

/* ------- variables -------------------------------------------------------------------------------------------------*/

static SignalEngine* g_signal_engine = nullptr;

/* ------- function implement ----------------------------------------------------------------------------------------*/

static int cmd_signal_start(const struct shell *sh, size_t argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    auto cmd = SignalCommand::make_start(CommandSource::Shell);
    auto result = g_signal_engine->handle_command(cmd);

    if (result.is_ok()) {
        shell_print(sh, "Signal started");
        return 0;
    } else {
        shell_error(sh, "Failed to start: error %d", static_cast<int>(result.error()));
        return -1;
    }
}

static int cmd_signal_stop(const struct shell *sh, size_t argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    auto cmd = SignalCommand::make_stop(CommandSource::Shell);
    auto result = g_signal_engine->handle_command(cmd);

    if (result.is_ok()) {
        shell_print(sh, "Signal stopped");
        return 0;
    } else {
        shell_error(sh, "Failed to stop: error %d", static_cast<int>(result.error()));
        return -1;
    }
}

static int cmd_signal_pause(const struct shell *sh, size_t argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    auto cmd = SignalCommand::make_pause(CommandSource::Shell);
    auto result = g_signal_engine->handle_command(cmd);

    if (result.is_ok()) {
        shell_print(sh, "Signal paused");
        return 0;
    } else {
        shell_error(sh, "Failed to pause: error %d", static_cast<int>(result.error()));
        return -1;
    }
}

static int cmd_signal_resume(const struct shell *sh, size_t argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    auto cmd = SignalCommand::make_resume(CommandSource::Shell);
    auto result = g_signal_engine->handle_command(cmd);

    if (result.is_ok()) {
        shell_print(sh, "Signal resumed");
        return 0;
    } else {
        shell_error(sh, "Failed to resume: error %d", static_cast<int>(result.error()));
        return -1;
    }
}

static int cmd_signal_freq(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_error(sh, "Usage: signal freq <hz>");
        return -1;
    }

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    uint32_t freq = static_cast<uint32_t>(atoi(argv[1]));
    auto cmd = SignalCommand::make_set_frequency(CommandSource::Shell, FrequencyHz{freq});
    auto result = g_signal_engine->handle_command(cmd);

    if (result.is_ok()) {
        shell_print(sh, "Frequency set to %u Hz", freq);
        return 0;
    } else {
        shell_error(sh, "Failed to set frequency: error %d", static_cast<int>(result.error()));
        return -1;
    }
}

static int cmd_signal_amp(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_error(sh, "Usage: signal amp <mv>");
        return -1;
    }

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    int32_t amp = atoi(argv[1]);
    auto cmd = SignalCommand::make_set_amplitude(CommandSource::Shell, VoltageMv{amp});
    auto result = g_signal_engine->handle_command(cmd);

    if (result.is_ok()) {
        shell_print(sh, "Amplitude set to %d mV", amp);
        return 0;
    } else {
        shell_error(sh, "Failed to set amplitude: error %d", static_cast<int>(result.error()));
        return -1;
    }
}

static int cmd_signal_status(const struct shell *sh, size_t argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    auto snapshot = g_signal_engine->snapshot();
    const char* state_str = "Unknown";

    switch (snapshot.state) {
        case SignalEngineState::Idle:    state_str = "Idle"; break;
        case SignalEngineState::Editing: state_str = "Editing"; break;
        case SignalEngineState::Arming:  state_str = "Arming"; break;
        case SignalEngineState::Running: state_str = "Running"; break;
        case SignalEngineState::Paused:  state_str = "Paused"; break;
        case SignalEngineState::Stopping: state_str = "Stopping"; break;
        case SignalEngineState::Fault:   state_str = "Fault"; break;
    }

    const char* waveform_str = "Unknown";
    switch (snapshot.active_profile.kind) {
        case WaveformKind::None:      waveform_str = "None"; break;
        case WaveformKind::Sine:      waveform_str = "Sine"; break;
        case WaveformKind::Square:    waveform_str = "Square"; break;
        case WaveformKind::Triangle:  waveform_str = "Triangle"; break;
        case WaveformKind::Sawtooth:  waveform_str = "Sawtooth"; break;
        case WaveformKind::Arbitrary: waveform_str = "Arbitrary"; break;
    }

    shell_print(sh, "Signal Engine Status:");
    shell_print(sh, "  State: %s", state_str);
    shell_print(sh, "  Waveform: %s", waveform_str);
    shell_print(sh, "  Frequency: %u Hz", snapshot.active_profile.frequency.value);
    shell_print(sh, "  Amplitude: %d mV", snapshot.active_profile.amplitude.value);
    shell_print(sh, "  Offset: %d mV", snapshot.active_profile.offset.value);
    shell_print(sh, "  Sample Rate: %u Hz", snapshot.active_profile.sample_rate.value);
    shell_print(sh, "  Commands: %u", snapshot.command_count);

    return 0;
}

/* ------- shell command definitions ----------------------------------------------------------------------------------*/

SHELL_STATIC_SUBCMD_SET_CREATE(sub_signal,
    SHELL_CMD(start, NULL, "Start signal output", cmd_signal_start),
    SHELL_CMD(stop, NULL, "Stop signal output", cmd_signal_stop),
    SHELL_CMD(pause, NULL, "Pause signal output", cmd_signal_pause),
    SHELL_CMD(resume, NULL, "Resume signal output", cmd_signal_resume),
    SHELL_CMD(freq, NULL, "Set frequency (Hz)", cmd_signal_freq),
    SHELL_CMD(amp, NULL, "Set amplitude (mV)", cmd_signal_amp),
    SHELL_CMD(status, NULL, "Show signal status", cmd_signal_status),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(signal, &sub_signal, "Signal generator commands", NULL);

/* ------- public functions ------------------------------------------------------------------------------------------*/

void register_signal_shell_commands(SignalEngine& engine)
{
    g_signal_engine = &engine;
    LOG_INF("Signal shell commands registered");
}

} // namespace omnigen
