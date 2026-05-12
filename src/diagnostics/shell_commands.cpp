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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/debug/cpu_load.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/mem_stats.h>
#include <zephyr/sys/sys_heap.h>

#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS) && (K_HEAP_MEM_POOL_SIZE > 0)
extern "C" struct k_heap _system_heap;
#endif

LOG_MODULE_REGISTER(signal_shell, CONFIG_LOG_DEFAULT_LEVEL);

namespace omnigen {

/* ------- variables -------------------------------------------------------------------------------------------------*/

static SignalEngine* g_signal_engine = nullptr;

struct SysResThreadDumpContext {
    const struct shell* sh{nullptr};
    uint32_t thread_count{0U};
#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
    k_thread_runtime_stats_t all_stats{};
    bool runtime_stats_ready{false};
#endif
};

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

static uint32_t ratio_to_permille_u64(uint64_t numerator, uint64_t denominator) {
    if (denominator == 0ULL) {
        return 0U;
    }

    uint64_t scaled = (numerator * 1000ULL) / denominator;
    if (scaled > 1000ULL) {
        scaled = 1000ULL;
    }

    return static_cast<uint32_t>(scaled);
}

static uint32_t thread_cpu_permille(const SysResThreadDumpContext& ctx, k_tid_t tid) {
#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
    if (!ctx.runtime_stats_ready || ctx.all_stats.execution_cycles == 0ULL) {
        return 0U;
    }

    k_thread_runtime_stats_t thread_stats{};
    if (k_thread_runtime_stats_get(tid, &thread_stats) != 0) {
        return 0U;
    }

    return ratio_to_permille_u64(thread_stats.execution_cycles, ctx.all_stats.execution_cycles);
#else
    ARG_UNUSED(ctx);
    ARG_UNUSED(tid);
    return 0U;
#endif
}

static void dump_thread_resource(const struct k_thread* cthread, void* user_data) {
    auto* ctx = static_cast<SysResThreadDumpContext*>(user_data);
    if (ctx == nullptr || ctx->sh == nullptr || cthread == nullptr) {
        return;
    }

    struct k_thread* thread = const_cast<struct k_thread*>(cthread);
    ctx->thread_count++;

    const char* tname = k_thread_name_get(thread);
    if (tname == nullptr) {
        tname = "NA";
    }

    char state_buf[32];
    const char* state = k_thread_state_str(thread, state_buf, sizeof(state_buf));
    const int priority = k_thread_priority_get(thread);
    const uint32_t cpu_pm = thread_cpu_permille(*ctx, thread);

    size_t stack_size = 0U;
#if defined(CONFIG_THREAD_STACK_INFO)
    stack_size = thread->stack_info.size;
#endif

    size_t stack_unused = 0U;
    int stack_rc        = -ENOTSUP;
#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
    stack_rc = k_thread_stack_space_get(thread, &stack_unused);
#endif

    if (stack_rc == 0 && stack_size > 0U) {
        const size_t stack_used = stack_size - stack_unused;
        const uint32_t stack_pm = static_cast<uint32_t>((stack_used * 1000U) / stack_size);
        shell_print(ctx->sh,
                    "  [%02u] %s tid=%p prio=%d state=%s cpu=%u.%u%% stack=%zu/%zu (%u.%u%%)",
                    ctx->thread_count, tname, static_cast<void*>(thread), priority, state, cpu_pm / 10U, cpu_pm % 10U,
                    stack_used, stack_size, stack_pm / 10U, stack_pm % 10U);
        return;
    }

    if (stack_size > 0U) {
        shell_print(ctx->sh, "  [%02u] %s tid=%p prio=%d state=%s cpu=%u.%u%% stack=?/%zu", ctx->thread_count, tname,
                    static_cast<void*>(thread), priority, state, cpu_pm / 10U, cpu_pm % 10U, stack_size);
        return;
    }

    shell_print(ctx->sh, "  [%02u] %s tid=%p prio=%d state=%s cpu=%u.%u%% stack=n/a", ctx->thread_count, tname,
                static_cast<void*>(thread), priority, state, cpu_pm / 10U, cpu_pm % 10U);
}

/* ------- command handlers ------------------------------------------------------------------------------------------*/

static int cmd_signal_start(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    auto cmd    = SignalCommand::make_start(CommandSource::Shell);
    auto result = g_signal_engine->handle_command(cmd);

    if (result.is_ok()) {
        shell_print(sh, "Signal started");
        return 0;
    } else {
        shell_error(sh, "Failed to start: error %d", static_cast<int>(result.error()));
        return -1;
    }
}

static int cmd_signal_stop(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    auto cmd    = SignalCommand::make_stop(CommandSource::Shell);
    auto result = g_signal_engine->handle_command(cmd);

    if (result.is_ok()) {
        shell_print(sh, "Signal stopped");
        return 0;
    } else {
        shell_error(sh, "Failed to stop: error %d", static_cast<int>(result.error()));
        return -1;
    }
}

static int cmd_signal_pause(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    auto cmd    = SignalCommand::make_pause(CommandSource::Shell);
    auto result = g_signal_engine->handle_command(cmd);

    if (result.is_ok()) {
        shell_print(sh, "Signal paused");
        return 0;
    } else {
        shell_error(sh, "Failed to pause: error %d", static_cast<int>(result.error()));
        return -1;
    }
}

static int cmd_signal_resume(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    auto cmd    = SignalCommand::make_resume(CommandSource::Shell);
    auto result = g_signal_engine->handle_command(cmd);

    if (result.is_ok()) {
        shell_print(sh, "Signal resumed");
        return 0;
    } else {
        shell_error(sh, "Failed to resume: error %d", static_cast<int>(result.error()));
        return -1;
    }
}

static int cmd_signal_freq(const struct shell* sh, size_t argc, char** argv) {
    if (argc < 2) {
        shell_error(sh, "Usage: signal freq <hz>");
        return -1;
    }

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    char* end  = nullptr;
    errno      = 0;
    float freq = strtof(argv[1], &end);
    if (end == argv[1] || *end != '\0' || errno != 0) {
        shell_error(sh, "Invalid frequency value: %s", argv[1]);
        return -1;
    }

    auto cmd    = SignalCommand::make_set_frequency(CommandSource::Shell, FrequencyHz{static_cast<uint32_t>(freq)});
    auto result = g_signal_engine->handle_command(cmd);

    if (result.is_ok()) {
        shell_print(sh, "Frequency set to %u Hz", static_cast<uint32_t>(freq));
        return 0;
    } else {
        shell_error(sh, "Failed to set frequency: error %d", static_cast<int>(result.error()));
        return -1;
    }
}

static int cmd_signal_amp(const struct shell* sh, size_t argc, char** argv) {
    if (argc < 2) {
        shell_error(sh, "Usage: signal amp <mv>");
        return -1;
    }

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    int32_t amp = atoi(argv[1]);
    auto cmd    = SignalCommand::make_set_amplitude(CommandSource::Shell, VoltageMv{amp});
    auto result = g_signal_engine->handle_command(cmd);

    if (result.is_ok()) {
        shell_print(sh, "Amplitude set to %d mV", amp);
        return 0;
    } else {
        shell_error(sh, "Failed to set amplitude: error %d", static_cast<int>(result.error()));
        return -1;
    }
}

static int cmd_signal_status(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    if (!g_signal_engine) {
        shell_error(sh, "Signal engine not initialized");
        return -1;
    }

    auto snapshot        = g_signal_engine->snapshot();
    const char* state_str = state_to_string(snapshot.state);
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

static int cmd_sysres(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    const uint32_t uptime_ms = k_uptime_get_32();
    shell_print(sh, "System Resource Snapshot");
    shell_print(sh, "- Uptime: %u ms (%u s)", uptime_ms, uptime_ms / 1000U);

#if defined(CONFIG_CPU_LOAD)
    const int cpu_pm = cpu_load_get(false);
    if (cpu_pm >= 0) {
        shell_print(sh, "- CPU Load: %u.%u%% (%d/1000)", static_cast<unsigned int>(cpu_pm / 10),
                    static_cast<unsigned int>(cpu_pm % 10), cpu_pm);
    } else {
        shell_print(sh, "- CPU Load: unavailable (err %d)", cpu_pm);
    }
#else
    shell_print(sh, "- CPU Load: disabled (CONFIG_CPU_LOAD=n)");
#endif

    SysResThreadDumpContext thread_ctx{};
    thread_ctx.sh = sh;

#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
    if (k_thread_runtime_stats_all_get(&thread_ctx.all_stats) == 0) {
        thread_ctx.runtime_stats_ready = true;
        const uint32_t sched_pm =
            ratio_to_permille_u64(thread_ctx.all_stats.total_cycles, thread_ctx.all_stats.execution_cycles);
        shell_print(sh, "- Scheduler Active: %u.%u%%", sched_pm / 10U, sched_pm % 10U);
    } else {
        shell_print(sh, "- Scheduler Active: unavailable");
    }
#else
    shell_print(sh, "- Scheduler Active: disabled (CONFIG_THREAD_RUNTIME_STATS/CONFIG_SCHED_THREAD_USAGE)");
#endif

#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS) && (K_HEAP_MEM_POOL_SIZE > 0)
    struct sys_memory_stats heap_stats{};
    const int heap_rc = sys_heap_runtime_stats_get(&_system_heap.heap, &heap_stats);
    if (heap_rc == 0) {
        const size_t heap_total = heap_stats.free_bytes + heap_stats.allocated_bytes;
        shell_print(sh, "- Heap: allocated=%zu free=%zu max=%zu total=%zu", heap_stats.allocated_bytes,
                    heap_stats.free_bytes, heap_stats.max_allocated_bytes, heap_total);
    } else {
        shell_print(sh, "- Heap: stats unavailable (err %d)", heap_rc);
    }
#else
    shell_print(sh, "- Heap: disabled (CONFIG_SYS_HEAP_RUNTIME_STATS=n or HEAP_MEM_POOL_SIZE=0)");
#endif

#if defined(CONFIG_THREAD_MONITOR)
    shell_print(sh, "- Threads:");
    k_thread_foreach_unlocked(dump_thread_resource, &thread_ctx);
    shell_print(sh, "- Thread Count: %u", thread_ctx.thread_count);
#else
    shell_print(sh, "- Threads: disabled (CONFIG_THREAD_MONITOR=n)");
#endif

    return 0;
}

/* ------- shell command definitions ----------------------------------------------------------------------------------*/

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_signal, SHELL_CMD(start, NULL, "Start signal output", cmd_signal_start),
    SHELL_CMD(stop, NULL, "Stop signal output", cmd_signal_stop),
    SHELL_CMD(pause, NULL, "Pause signal output", cmd_signal_pause),
    SHELL_CMD(resume, NULL, "Resume signal output", cmd_signal_resume),
    SHELL_CMD(freq, NULL, "Set frequency (Hz)", cmd_signal_freq),
    SHELL_CMD(amp, NULL, "Set amplitude (mV)", cmd_signal_amp),
    SHELL_CMD(status, NULL, "Show signal status", cmd_signal_status), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(signal, &sub_signal, "Signal generator commands", NULL);
SHELL_CMD_REGISTER(sysres, NULL, "Show CPU/thread/memory resource usage", cmd_sysres);

/* ------- public functions ------------------------------------------------------------------------------------------*/

void register_signal_shell_commands(SignalEngine& engine) {
    g_signal_engine = &engine;
    LOG_INF("Signal shell commands registered");
}

} // namespace omnigen
