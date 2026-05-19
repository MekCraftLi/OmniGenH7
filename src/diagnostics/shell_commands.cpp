/**
 *******************************************************************************
 * @file    shell_commands.cpp
 * @brief   Shell commands implementation for signal engine control
 *******************************************************************************
 * @attention
 *
 * Implements Zephyr shell commands for the signal engine. Hardware diagnostics
 * live in dedicated shell command translation units.
 *
 *******************************************************************************
 * @note
 *
 * Usage examples:
 * - signal start
 * - signal stop
 * - signal freq 1000
 * - signal amp 1500
 * - signal status
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.1
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "diagnostics/shell_commands.hpp"

#include <errno.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(signal_shell, CONFIG_LOG_DEFAULT_LEVEL);

namespace omnigen {

/*-------- 2. variables ----------------------------------------------------------------------------------------------*/

static CommandBusPort* g_command_bus = nullptr;
static RequestBusPort* g_request_bus = nullptr;

/*-------- 3. internal helpers ---------------------------------------------------------------------------------------*/

static Result<void> submit_signal_command(const SignalCommand& command)
{
    if (g_command_bus == nullptr) {
        return ErrorCode::InvalidState;
    }

    return g_command_bus->submit(AppCommand::make_signal(command));
}

static const char* state_to_string(SignalEngineState state)
{
    switch (state) {
        case SignalEngineState::Idle:
            return "Idle";
        case SignalEngineState::Editing:
            return "Editing";
        case SignalEngineState::Arming:
            return "Arming";
        case SignalEngineState::Running:
            return "Running";
        case SignalEngineState::Paused:
            return "Paused";
        case SignalEngineState::Stopping:
            return "Stopping";
        case SignalEngineState::Fault:
            return "Fault";
        default:
            return "Unknown";
    }
}

static const char* waveform_to_string(WaveformKind kind)
{
    switch (kind) {
        case WaveformKind::None:
            return "None";
        case WaveformKind::Sine:
            return "Sine";
        case WaveformKind::Square:
            return "Square";
        case WaveformKind::Triangle:
            return "Triangle";
        case WaveformKind::Sawtooth:
            return "Sawtooth";
        case WaveformKind::Arbitrary:
            return "Arbitrary";
        default:
            return "Unknown";
    }
}

/*-------- 4. command handlers ---------------------------------------------------------------------------------------*/

static int cmd_signal_start(const struct shell* sh, size_t argc, char** argv)
{
    (void)argc;
    (void)argv;

    if (g_command_bus == nullptr) {
        shell_error(sh, "Command bus not initialized");
        return -1;
    }

    auto result = submit_signal_command(SignalCommand::make_start(CommandSource::Shell));
    if (result.is_ok()) {
        shell_print(sh, "Signal started");
        return 0;
    }

    shell_error(sh, "Failed to start: error %d", static_cast<int>(result.error()));
    return -1;
}

static int cmd_signal_stop(const struct shell* sh, size_t argc, char** argv)
{
    (void)argc;
    (void)argv;

    if (g_command_bus == nullptr) {
        shell_error(sh, "Command bus not initialized");
        return -1;
    }

    auto result = submit_signal_command(SignalCommand::make_stop(CommandSource::Shell));
    if (result.is_ok()) {
        shell_print(sh, "Signal stopped");
        return 0;
    }

    shell_error(sh, "Failed to stop: error %d", static_cast<int>(result.error()));
    return -1;
}

static int cmd_signal_pause(const struct shell* sh, size_t argc, char** argv)
{
    (void)argc;
    (void)argv;

    if (g_command_bus == nullptr) {
        shell_error(sh, "Command bus not initialized");
        return -1;
    }

    auto result = submit_signal_command(SignalCommand::make_pause(CommandSource::Shell));
    if (result.is_ok()) {
        shell_print(sh, "Signal paused");
        return 0;
    }

    shell_error(sh, "Failed to pause: error %d", static_cast<int>(result.error()));
    return -1;
}

static int cmd_signal_resume(const struct shell* sh, size_t argc, char** argv)
{
    (void)argc;
    (void)argv;

    if (g_command_bus == nullptr) {
        shell_error(sh, "Command bus not initialized");
        return -1;
    }

    auto result = submit_signal_command(SignalCommand::make_resume(CommandSource::Shell));
    if (result.is_ok()) {
        shell_print(sh, "Signal resumed");
        return 0;
    }

    shell_error(sh, "Failed to resume: error %d", static_cast<int>(result.error()));
    return -1;
}

static int cmd_signal_freq(const struct shell* sh, size_t argc, char** argv)
{
    if (argc < 2U) {
        shell_error(sh, "Usage: signal freq <hz>");
        return -1;
    }

    if (g_command_bus == nullptr) {
        shell_error(sh, "Command bus not initialized");
        return -1;
    }

    char* end  = nullptr;
    errno      = 0;
    float freq = strtof(argv[1], &end);
    if (end == argv[1] || *end != '\0' || errno != 0 || freq < 0.0F) {
        shell_error(sh, "Invalid frequency value: %s", argv[1]);
        return -1;
    }

    auto result = submit_signal_command(
        SignalCommand::make_set_frequency(CommandSource::Shell, FrequencyHz{static_cast<uint32_t>(freq)}));
    if (result.is_ok()) {
        shell_print(sh, "Frequency set to %u Hz", static_cast<uint32_t>(freq));
        return 0;
    }

    shell_error(sh, "Failed to set frequency: error %d", static_cast<int>(result.error()));
    return -1;
}

static int cmd_signal_amp(const struct shell* sh, size_t argc, char** argv)
{
    if (argc < 2U) {
        shell_error(sh, "Usage: signal amp <mv>");
        return -1;
    }

    if (g_command_bus == nullptr) {
        shell_error(sh, "Command bus not initialized");
        return -1;
    }

    int32_t amp = atoi(argv[1]);
    auto result = submit_signal_command(SignalCommand::make_set_amplitude(CommandSource::Shell, VoltageMv{amp}));
    if (result.is_ok()) {
        shell_print(sh, "Amplitude set to %d mV", amp);
        return 0;
    }

    shell_error(sh, "Failed to set amplitude: error %d", static_cast<int>(result.error()));
    return -1;
}

static int cmd_signal_status(const struct shell* sh, size_t argc, char** argv)
{
    (void)argc;
    (void)argv;

    if (g_request_bus == nullptr) {
        shell_error(sh, "Request bus not initialized");
        return -1;
    }

    AppResponse response{};
    auto result = g_request_bus->request(AppRequest::make(AppRequestKind::SignalSnapshot), response);
    if (result.is_error()) {
        shell_error(sh, "Failed to query signal status: error %d", static_cast<int>(result.error()));
        return -1;
    }

    const auto snapshot      = response.signal;
    const char* state_str    = state_to_string(snapshot.state);
    const char* waveform_str = waveform_to_string(snapshot.active_profile.kind);

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

/*-------- 5. shell command definitions ------------------------------------------------------------------------------*/

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_signal,
    SHELL_CMD(start, NULL, "Start signal output", cmd_signal_start),
    SHELL_CMD(stop, NULL, "Stop signal output", cmd_signal_stop),
    SHELL_CMD(pause, NULL, "Pause signal output", cmd_signal_pause),
    SHELL_CMD(resume, NULL, "Resume signal output", cmd_signal_resume),
    SHELL_CMD(freq, NULL, "Set frequency (Hz)", cmd_signal_freq),
    SHELL_CMD(amp, NULL, "Set amplitude (mV)", cmd_signal_amp),
    SHELL_CMD(status, NULL, "Show signal status", cmd_signal_status),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(signal, &sub_signal, "Signal generator commands", NULL);

/*-------- 6. public functions ---------------------------------------------------------------------------------------*/

void register_signal_shell_commands(CommandBusPort& command_bus, RequestBusPort& request_bus)
{
    g_command_bus = &command_bus;
    g_request_bus = &request_bus;
    LOG_INF("Signal shell commands registered");
}

} // namespace omnigen
