/**
 *******************************************************************************
 * @file    shell_commands.cpp
 * @brief   Shell commands implementation for signal engine testing
 *******************************************************************************
 */

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "diagnostics/shell_commands.hpp"

#include <stdlib.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(signal_shell, CONFIG_LOG_DEFAULT_LEVEL);

namespace omnigen {

/* ------- variables -------------------------------------------------------------------------------------------------*/

static SignalEngine* g_signal_engine = nullptr;

/* ------- helpers ---------------------------------------------------------------------------------------------------*/

static const char* state_to_string(SignalEngineState state) {
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

static const char* waveform_to_string(WaveformKind kind) {
    switch (kind) {
        case WaveformKind::None:
            return "None";
        case WaveformKind::Sine:
            return "Sine";
        case WaveformKind::Square:
            return "Square";
        case WaveformKind::Pwm:
            return "PWM";
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

static bool ensure_engine(const struct shell* sh) {
    if (g_signal_engine == nullptr) {
        shell_error(sh, "Signal engine not initialized");
        return false;
    }
    return true;
}

static bool parse_frequency_mhz(const char* text, uint32_t* out_mhz) {
    if (text == nullptr || out_mhz == nullptr || text[0] == '\0') {
        return false;
    }

    uint64_t hz = 0U;
    uint32_t frac_mhz = 0U;
    uint32_t frac_scale = 100U;
    bool seen_digit = false;
    bool seen_dot = false;

    for (const char* p = text; *p != '\0'; ++p) {
        if (*p == '.') {
            if (seen_dot) {
                return false;
            }
            seen_dot = true;
            continue;
        }

        if (*p < '0' || *p > '9') {
            return false;
        }

        seen_digit = true;
        const uint32_t digit = static_cast<uint32_t>(*p - '0');
        if (!seen_dot) {
            hz = hz * 10U + digit;
            if (hz > 4294967U) {
                return false;
            }
        } else if (frac_scale > 0U) {
            frac_mhz += digit * frac_scale;
            frac_scale /= 10U;
        }
    }

    if (!seen_digit) {
        return false;
    }

    *out_mhz = static_cast<uint32_t>(hz * 1000U + frac_mhz);
    return true;
}

static void print_frequency_hz(const struct shell* sh, const char* label, FrequencyHz frequency) {
    const uint32_t hz = frequency.value / 1000U;
    const uint32_t mhz = frequency.value % 1000U;

    if (mhz == 0U) {
        shell_print(sh, "%s%u Hz", label, hz);
    } else {
        shell_print(sh, "%s%u.%03u Hz", label, hz, mhz);
    }
}

/* ------- command handlers ------------------------------------------------------------------------------------------*/

static int cmd_signal_start(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;
    if (!ensure_engine(sh)) {
        return -1;
    }

    auto result = g_signal_engine->handle_command(SignalCommand::make_start(CommandSource::Shell));
    if (result.is_error()) {
        shell_error(sh, "Failed to start: error %d", static_cast<int>(result.error()));
        return -1;
    }
    shell_print(sh, "Signal started");
    return 0;
}

static int cmd_signal_stop(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;
    if (!ensure_engine(sh)) {
        return -1;
    }

    auto result = g_signal_engine->handle_command(SignalCommand::make_stop(CommandSource::Shell));
    if (result.is_error()) {
        shell_error(sh, "Failed to stop: error %d", static_cast<int>(result.error()));
        return -1;
    }
    shell_print(sh, "Signal stopped");
    return 0;
}

static int cmd_signal_pause(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;
    if (!ensure_engine(sh)) {
        return -1;
    }

    auto result = g_signal_engine->handle_command(SignalCommand::make_pause(CommandSource::Shell));
    if (result.is_error()) {
        shell_error(sh, "Failed to pause: error %d", static_cast<int>(result.error()));
        return -1;
    }
    shell_print(sh, "Signal paused");
    return 0;
}

static int cmd_signal_resume(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;
    if (!ensure_engine(sh)) {
        return -1;
    }

    auto result = g_signal_engine->handle_command(SignalCommand::make_resume(CommandSource::Shell));
    if (result.is_error()) {
        shell_error(sh, "Failed to resume: error %d", static_cast<int>(result.error()));
        return -1;
    }
    shell_print(sh, "Signal resumed");
    return 0;
}

static int cmd_signal_freq(const struct shell* sh, size_t argc, char** argv) {
    if (argc < 2) {
        shell_error(sh, "Usage: signal freq <hz>");
        return -1;
    }
    if (!ensure_engine(sh)) {
        return -1;
    }

    uint32_t freq_mhz = 0U;
    if (!parse_frequency_mhz(argv[1], &freq_mhz)) {
        shell_error(sh, "Invalid frequency: use Hz, e.g. 1000 or 1234.567");
        return -1;
    }

    auto result   = g_signal_engine->handle_command(
        SignalCommand::make_set_frequency(CommandSource::Shell, FrequencyHz{freq_mhz}));
    if (result.is_error()) {
        shell_error(sh, "Failed to set frequency: error %d", static_cast<int>(result.error()));
        return -1;
    }
    print_frequency_hz(sh, "Frequency set to ", FrequencyHz{freq_mhz});
    return 0;
}

static int cmd_signal_sample_rate(const struct shell* sh, size_t argc, char** argv) {
    if (argc < 2) {
        shell_error(sh, "Usage: signal sample_rate <hz>");
        return -1;
    }
    if (!ensure_engine(sh)) {
        return -1;
    }

    uint32_t sample_rate = static_cast<uint32_t>(atoi(argv[1]));
    auto result          = g_signal_engine->handle_command(
        SignalCommand::make_set_sample_rate(CommandSource::Shell, SampleRateHz{sample_rate}));
    if (result.is_error()) {
        shell_error(sh, "Failed to set sample_rate: error %d", static_cast<int>(result.error()));
        return -1;
    }
    shell_print(sh, "Sample rate set to %u Hz", sample_rate);
    return 0;
}

static int cmd_signal_amp(const struct shell* sh, size_t argc, char** argv) {
    if (argc < 2) {
        shell_error(sh, "Usage: signal amp <mv>");
        return -1;
    }
    if (!ensure_engine(sh)) {
        return -1;
    }

    int32_t amp = atoi(argv[1]);
    auto result = g_signal_engine->handle_command(
        SignalCommand::make_set_amplitude(CommandSource::Shell, VoltageMv{amp}));
    if (result.is_error()) {
        shell_error(sh, "Failed to set amplitude: error %d", static_cast<int>(result.error()));
        return -1;
    }
    shell_print(sh, "Amplitude set to %d mV", amp);
    return 0;
}

static int cmd_signal_offset(const struct shell* sh, size_t argc, char** argv) {
    if (argc < 2) {
        shell_error(sh, "Usage: signal offset <mv>");
        return -1;
    }
    if (!ensure_engine(sh)) {
        return -1;
    }

    int32_t offset = atoi(argv[1]);
    auto result    = g_signal_engine->handle_command(
        SignalCommand::make_set_offset(CommandSource::Shell, VoltageMv{offset}));
    if (result.is_error()) {
        shell_error(sh, "Failed to set offset: error %d", static_cast<int>(result.error()));
        return -1;
    }
    shell_print(sh, "Offset set to %d mV", offset);
    return 0;
}

static int cmd_signal_duty(const struct shell* sh, size_t argc, char** argv) {
    if (argc < 2) {
        shell_error(sh, "Usage: signal duty <permille 0..1000>");
        return -1;
    }
    if (!ensure_engine(sh)) {
        return -1;
    }

    int32_t duty_raw = atoi(argv[1]);
    if (duty_raw < 0 || duty_raw > 1000) {
        shell_error(sh, "Duty out of range: 0..1000");
        return -1;
    }

    auto result = g_signal_engine->handle_command(
        SignalCommand::make_set_duty(CommandSource::Shell, DutyPermille{static_cast<uint16_t>(duty_raw)}));
    if (result.is_error()) {
        shell_error(sh, "Failed to set duty: error %d", static_cast<int>(result.error()));
        return -1;
    }
    shell_print(sh, "Duty set to %d/1000", duty_raw);
    return 0;
}

static int cmd_signal_waveform(const struct shell* sh, size_t argc, char** argv) {
    if (argc < 2) {
        shell_error(sh, "Usage: signal waveform <sine|triangle|saw|square|pwm>");
        return -1;
    }
    if (!ensure_engine(sh)) {
        return -1;
    }

    WaveformKind kind = WaveformKind::None;
    if (strcmp(argv[1], "sine") == 0) {
        kind = WaveformKind::Sine;
    } else if (strcmp(argv[1], "triangle") == 0) {
        kind = WaveformKind::Triangle;
    } else if (strcmp(argv[1], "saw") == 0 || strcmp(argv[1], "sawtooth") == 0) {
        kind = WaveformKind::Sawtooth;
    } else if (strcmp(argv[1], "square") == 0) {
        kind = WaveformKind::Square;
    } else if (strcmp(argv[1], "pwm") == 0) {
        kind = WaveformKind::Pwm;
    } else {
        shell_error(sh, "Unknown waveform: %s", argv[1]);
        return -1;
    }

    auto result = g_signal_engine->handle_command(
        SignalCommand::make_set_waveform(CommandSource::Shell, kind));
    if (result.is_error()) {
        shell_error(sh, "Failed to set waveform: error %d", static_cast<int>(result.error()));
        return -1;
    }
    shell_print(sh, "Waveform set to %s", argv[1]);
    return 0;
}

static int cmd_signal_help(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    shell_print(sh, "signal commands:");
    shell_print(sh, "  signal start");
    shell_print(sh, "  signal stop");
    shell_print(sh, "  signal pause");
    shell_print(sh, "  signal resume");
    shell_print(sh, "  signal status");
    shell_print(sh, "  signal waveform <sine|triangle|saw|square|pwm>");
    shell_print(sh, "  signal freq <hz>");
    shell_print(sh, "  signal sample_rate <hz>");
    shell_print(sh, "  signal amp <mv>");
    shell_print(sh, "  signal offset <mv>");
    shell_print(sh, "  signal duty <0..1000>");
    shell_print(sh, "examples:");
    shell_print(sh, "  signal waveform sine");
    shell_print(sh, "  signal freq 1000.5");
    shell_print(sh, "  signal sample_rate 64032");
    shell_print(sh, "  signal amp 2000");
    shell_print(sh, "  signal offset 1650");
    shell_print(sh, "  signal duty 500");
    return 0;
}

static int cmd_signal_status(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;
    if (!ensure_engine(sh)) {
        return -1;
    }

    const auto snapshot = g_signal_engine->snapshot();
    shell_print(sh, "Signal Engine Status:");
    shell_print(sh, "- State: %s", state_to_string(snapshot.state));
    shell_print(sh, "- Waveform: %s", waveform_to_string(snapshot.active_profile.kind));
    print_frequency_hz(sh, "- Frequency: ", snapshot.active_profile.frequency);
    shell_print(sh, "- Sample Rate: %u Hz", snapshot.active_profile.sample_rate.value);
    shell_print(sh, "- Samples/Cycle: %u", snapshot.active_profile.samples_per_cycle);
    shell_print(sh, "- Amplitude: %d mV", snapshot.active_profile.amplitude.value);
    shell_print(sh, "- Offset: %d mV", snapshot.active_profile.offset.value);
    shell_print(sh, "- Duty: %u/1000", snapshot.active_profile.duty.value);
    shell_print(sh, "- Commands: %u", snapshot.command_count);
    return 0;
}

/* ------- shell command definitions ---------------------------------------------------------------------------------*/

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_signal,
    SHELL_CMD(start, NULL, "Start signal output", cmd_signal_start),
    SHELL_CMD(stop, NULL, "Stop signal output", cmd_signal_stop),
    SHELL_CMD(pause, NULL, "Pause signal output", cmd_signal_pause),
    SHELL_CMD(resume, NULL, "Resume signal output", cmd_signal_resume),
    SHELL_CMD(freq, NULL, "Set frequency (Hz)", cmd_signal_freq),
    SHELL_CMD(sample_rate, NULL, "Set sample rate (Hz)", cmd_signal_sample_rate),
    SHELL_CMD(amp, NULL, "Set amplitude (mV)", cmd_signal_amp),
    SHELL_CMD(offset, NULL, "Set offset (mV)", cmd_signal_offset),
    SHELL_CMD(duty, NULL, "Set duty (permille)", cmd_signal_duty),
    SHELL_CMD(waveform, NULL, "Set waveform type", cmd_signal_waveform),
    SHELL_CMD(help, NULL, "Show signal command help", cmd_signal_help),
    SHELL_CMD(status, NULL, "Show signal status", cmd_signal_status),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(signal, &sub_signal, "Signal generator commands", NULL);

/* ------- public functions ------------------------------------------------------------------------------------------*/

void register_signal_shell_commands(SignalEngine& engine) {
    g_signal_engine = &engine;
    LOG_INF("Signal shell commands registered");
}

} // namespace omnigen
