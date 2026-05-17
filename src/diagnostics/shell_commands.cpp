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

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "diagnostics/shell_commands.hpp"
#include "drivers/ili9481_support.h"
#include "drivers/w25q64_support.h"

#include <errno.h>
#include <limits.h>
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

/*-------- 2. variables ----------------------------------------------------------------------------------------------*/

static CommandBusPort* g_command_bus = nullptr;
static RequestBusPort* g_request_bus = nullptr;
static constexpr size_t k_nor_shell_line_bytes = 16U;
static constexpr size_t k_nor_shell_rw_chunk   = 256U;
static constexpr size_t k_nor_shell_max_read   = 4096U;

/**
 * @brief 系统资源线程遍历上下文。
 *
 * Shell 资源诊断命令在遍历 Zephyr 线程列表时使用该结构保存输出目标、线程计数
 * 和可选的运行时间统计基准。
 */
struct SysResThreadDumpContext {
    const struct shell* sh{nullptr};
    uint32_t thread_count{0U};
#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
    k_thread_runtime_stats_t all_stats{};
    bool runtime_stats_ready{false};
#endif
};

/**
 * @brief 系统资源快照结构体。
 *
 * 保存一次 `sysres` 诊断采样得到的运行时间、CPU 负载、堆统计和线程数量等信息。
 * 条件编译字段只在对应 Zephyr 功能启用时存在。
 */
struct SysResSnapshot {
    uint32_t uptime_ms{0U};
#if defined(CONFIG_CPU_LOAD)
    int cpu_pm{-ENOTSUP};
#endif
#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
    bool scheduler_active_available{false};
    uint32_t scheduler_active_pm{0U};
#endif
#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS) && (K_HEAP_MEM_POOL_SIZE > 0)
    int heap_rc{-ENOTSUP};
    struct sys_memory_stats heap_stats{};
#endif
    uint32_t thread_count{0U};
};

static uint32_t g_sysres_sample_count = 0U;
#if defined(CONFIG_CPU_LOAD)
static int g_sysres_peak_cpu_pm = -ENOTSUP;
#endif
#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
static uint32_t g_sysres_peak_sched_pm = 0U;
#endif
static uint32_t g_sysres_peak_thread_count = 0U;

/*-------- 3. internal helpers ---------------------------------------------------------------------------------------*/

static Result<void> submit_signal_command(const SignalCommand& command)
{
    if (g_command_bus == nullptr) {
        return ErrorCode::InvalidState;
    }

    return g_command_bus->submit(AppCommand::make_signal(command));
}

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

static bool parse_u32_arg(const char* text, uint32_t* out_value) {
    if (text == nullptr || out_value == nullptr || text[0] == '\0') {
        return false;
    }

    char* end = nullptr;
    errno     = 0;
    unsigned long value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value > static_cast<unsigned long>(UINT32_MAX)) {
        return false;
    }

    *out_value = static_cast<uint32_t>(value);
    return true;
}

static bool parse_u8_arg(const char* text, uint8_t* out_value) {
    if (text == nullptr || out_value == nullptr || text[0] == '\0') {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    unsigned long value = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || value > static_cast<unsigned long>(UINT8_MAX)) {
        return false;
    }

    *out_value = static_cast<uint8_t>(value);
    return true;
}

static bool parse_u16_arg(const char* text, uint16_t* out_value) {
    if (text == nullptr || out_value == nullptr || text[0] == '\0') {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    unsigned long value = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || value > static_cast<unsigned long>(UINT16_MAX)) {
        return false;
    }

    *out_value = static_cast<uint16_t>(value);
    return true;
}

static bool parse_size_arg(const char* text, size_t* out_value) {
    if (text == nullptr || out_value == nullptr || text[0] == '\0') {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    unsigned long value = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0') {
        return false;
    }

    *out_value = static_cast<size_t>(value);
    return true;
}

static bool parse_addr_arg(const char* text, uint32_t* out_addr) {
    if (text == nullptr || out_addr == nullptr || text[0] == '\0') {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    unsigned long value = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || value > static_cast<unsigned long>(UINT32_MAX)) {
        return false;
    }

    *out_addr = static_cast<uint32_t>(value);
    return true;
}

static int nor_check_ready(const struct shell* sh) {
    if (!w25q64_support_ready()) {
        shell_error(sh, "NOR not ready. Re-run: nor probe");
        return -ENODEV;
    }
    return 0;
}

static int lcd_check_ready(const struct shell* sh) {
    if (!ili9481_support_ready()) {
        shell_error(sh, "LCD not ready. Re-run: lcd init");
        return -ENODEV;
    }
    return 0;
}

static void sysres_reset_window_and_peaks(void) {
    g_sysres_sample_count      = 0U;
    g_sysres_peak_thread_count = 0U;

#if defined(CONFIG_CPU_LOAD)
    /* cpu_load_get(true) both retrieves and resets the measurement window. */
    (void)cpu_load_get(true);
    g_sysres_peak_cpu_pm = -ENOTSUP;
#endif

#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
    g_sysres_peak_sched_pm = 0U;
#endif
}

static SysResSnapshot collect_sysres_snapshot(void) {
    SysResSnapshot snapshot{};
    snapshot.uptime_ms = k_uptime_get_32();

#if defined(CONFIG_CPU_LOAD)
    snapshot.cpu_pm = cpu_load_get(false);
#endif

    SysResThreadDumpContext thread_ctx{};
#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
    if (k_thread_runtime_stats_all_get(&thread_ctx.all_stats) == 0) {
        thread_ctx.runtime_stats_ready     = true;
        snapshot.scheduler_active_available = true;
        snapshot.scheduler_active_pm =
            ratio_to_permille_u64(thread_ctx.all_stats.total_cycles, thread_ctx.all_stats.execution_cycles);
    }
#endif

#if defined(CONFIG_THREAD_MONITOR)
    k_thread_foreach_unlocked(
        [](const struct k_thread* cthread, void* user_data) {
            ARG_UNUSED(cthread);
            auto* count = static_cast<uint32_t*>(user_data);
            if (count != nullptr) {
                (*count)++;
            }
        },
        &snapshot.thread_count);
#endif

#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS) && (K_HEAP_MEM_POOL_SIZE > 0)
    snapshot.heap_rc = sys_heap_runtime_stats_get(&_system_heap.heap, &snapshot.heap_stats);
#endif

    g_sysres_sample_count++;
    if (snapshot.thread_count > g_sysres_peak_thread_count) {
        g_sysres_peak_thread_count = snapshot.thread_count;
    }

#if defined(CONFIG_CPU_LOAD)
    if (snapshot.cpu_pm >= 0 && snapshot.cpu_pm > g_sysres_peak_cpu_pm) {
        g_sysres_peak_cpu_pm = snapshot.cpu_pm;
    }
#endif

#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
    if (snapshot.scheduler_active_available && snapshot.scheduler_active_pm > g_sysres_peak_sched_pm) {
        g_sysres_peak_sched_pm = snapshot.scheduler_active_pm;
    }
#endif

    return snapshot;
}

static void print_sysres_snapshot(const struct shell* sh, bool include_thread_table) {
    SysResSnapshot snapshot = collect_sysres_snapshot();

    shell_print(sh, "System Resource Snapshot");
    shell_print(sh, "- Uptime: %u ms (%u s)", snapshot.uptime_ms, snapshot.uptime_ms / 1000U);
    shell_print(sh, "- Window Samples: %u", g_sysres_sample_count);

#if defined(CONFIG_CPU_LOAD)
    if (snapshot.cpu_pm >= 0) {
        if (g_sysres_peak_cpu_pm >= 0) {
            shell_print(sh, "- CPU Load: %u.%u%% (peak %u.%u%%)", static_cast<unsigned int>(snapshot.cpu_pm / 10),
                        static_cast<unsigned int>(snapshot.cpu_pm % 10),
                        static_cast<unsigned int>(g_sysres_peak_cpu_pm / 10),
                        static_cast<unsigned int>(g_sysres_peak_cpu_pm % 10));
        } else {
            shell_print(sh, "- CPU Load: %u.%u%%", static_cast<unsigned int>(snapshot.cpu_pm / 10),
                        static_cast<unsigned int>(snapshot.cpu_pm % 10));
        }
    } else {
        shell_print(sh, "- CPU Load: unavailable (err %d)", snapshot.cpu_pm);
    }
#else
    shell_print(sh, "- CPU Load: disabled (CONFIG_CPU_LOAD=n)");
#endif

#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
    if (snapshot.scheduler_active_available) {
        shell_print(sh, "- Scheduler Active: %u.%u%% (peak %u.%u%%)", snapshot.scheduler_active_pm / 10U,
                    snapshot.scheduler_active_pm % 10U, g_sysres_peak_sched_pm / 10U, g_sysres_peak_sched_pm % 10U);
    } else {
        shell_print(sh, "- Scheduler Active: unavailable");
    }
#else
    shell_print(sh, "- Scheduler Active: disabled (CONFIG_THREAD_RUNTIME_STATS/CONFIG_SCHED_THREAD_USAGE)");
#endif

#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS) && (K_HEAP_MEM_POOL_SIZE > 0)
    if (snapshot.heap_rc == 0) {
        const size_t heap_total = snapshot.heap_stats.free_bytes + snapshot.heap_stats.allocated_bytes;
        shell_print(sh, "- Heap: allocated=%zu free=%zu max=%zu total=%zu", snapshot.heap_stats.allocated_bytes,
                    snapshot.heap_stats.free_bytes, snapshot.heap_stats.max_allocated_bytes, heap_total);
    } else {
        shell_print(sh, "- Heap: stats unavailable (err %d)", snapshot.heap_rc);
    }
#else
    shell_print(sh, "- Heap: disabled (CONFIG_SYS_HEAP_RUNTIME_STATS=n or HEAP_MEM_POOL_SIZE=0)");
#endif

    shell_print(sh, "- Thread Count: %u (peak %u)", snapshot.thread_count, g_sysres_peak_thread_count);

#if defined(CONFIG_THREAD_MONITOR)
    if (include_thread_table) {
        SysResThreadDumpContext thread_ctx{};
        thread_ctx.sh = sh;
#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
        if (k_thread_runtime_stats_all_get(&thread_ctx.all_stats) == 0) {
            thread_ctx.runtime_stats_ready = true;
        }
#endif
        shell_print(sh, "- Threads:");
        k_thread_foreach_unlocked(dump_thread_resource, &thread_ctx);
    }
#else
    ARG_UNUSED(include_thread_table);
    shell_print(sh, "- Threads: disabled (CONFIG_THREAD_MONITOR=n)");
#endif
}

/*-------- 4. command handlers ---------------------------------------------------------------------------------------*/

static int cmd_signal_start(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    if (g_command_bus == nullptr) {
        shell_error(sh, "Command bus not initialized");
        return -1;
    }

    auto cmd    = SignalCommand::make_start(CommandSource::Shell);
    auto result = submit_signal_command(cmd);

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

    if (g_command_bus == nullptr) {
        shell_error(sh, "Command bus not initialized");
        return -1;
    }

    auto cmd    = SignalCommand::make_stop(CommandSource::Shell);
    auto result = submit_signal_command(cmd);

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

    if (g_command_bus == nullptr) {
        shell_error(sh, "Command bus not initialized");
        return -1;
    }

    auto cmd    = SignalCommand::make_pause(CommandSource::Shell);
    auto result = submit_signal_command(cmd);

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

    if (g_command_bus == nullptr) {
        shell_error(sh, "Command bus not initialized");
        return -1;
    }

    auto cmd    = SignalCommand::make_resume(CommandSource::Shell);
    auto result = submit_signal_command(cmd);

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

    if (g_command_bus == nullptr) {
        shell_error(sh, "Command bus not initialized");
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
    auto result = submit_signal_command(cmd);

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

    if (g_command_bus == nullptr) {
        shell_error(sh, "Command bus not initialized");
        return -1;
    }

    int32_t amp = atoi(argv[1]);
    auto cmd    = SignalCommand::make_set_amplitude(CommandSource::Shell, VoltageMv{amp});
    auto result = submit_signal_command(cmd);

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

    if (g_request_bus == nullptr) {
        shell_error(sh, "Request bus not initialized");
        return -1;
    }

    AppResponse response{};
    auto request = AppRequest::make(AppRequestKind::SignalSnapshot);
    auto result = g_request_bus->request(request, response);
    if (result.is_error()) {
        shell_error(sh, "Failed to query signal status: error %d", static_cast<int>(result.error()));
        return -1;
    }

    auto snapshot        = response.signal;
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
    if (argc == 1U) {
        print_sysres_snapshot(sh, true);
        return 0;
    }

    if (strcmp(argv[1], "reset") == 0) {
        sysres_reset_window_and_peaks();
        shell_print(sh, "sysres window and peaks reset");
        return 0;
    }

    if (strcmp(argv[1], "watch") == 0) {
        if (argc < 3U || argc > 4U) {
            shell_error(sh, "Usage: sysres watch <period_ms> [count]");
            return -EINVAL;
        }

        uint32_t period_ms = 0U;
        if (!parse_u32_arg(argv[2], &period_ms) || period_ms == 0U) {
            shell_error(sh, "Invalid period_ms: %s", argv[2]);
            return -EINVAL;
        }

        uint32_t count = 10U;
        if (argc == 4U) {
            if (!parse_u32_arg(argv[3], &count) || count == 0U) {
                shell_error(sh, "Invalid count: %s", argv[3]);
                return -EINVAL;
            }
        }

        shell_print(sh, "sysres watch started: period=%u ms, count=%u", period_ms, count);
        for (uint32_t i = 0U; i < count; ++i) {
            shell_print(sh, "---- sample %u/%u ----", i + 1U, count);
            print_sysres_snapshot(sh, false);
            if (i + 1U < count) {
                k_msleep(period_ms);
            }
        }
        shell_print(sh, "sysres watch completed");
        return 0;
    }

    shell_error(sh, "Unknown argument: %s", argv[1]);
    shell_error(sh, "Usage: sysres | sysres reset | sysres watch <period_ms> [count]");
    return -EINVAL;
}

static int cmd_nor_probe(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    int ret = w25q64_support_init();
    if (ret != 0) {
        shell_error(sh, "NOR init failed: %d", ret);
        return ret;
    }

    shell_print(sh, "NOR probe success");
    return 0;
}

static int cmd_lcd_init(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    int ret = ili9481_support_init();
    if (ret != 0) {
        shell_error(sh, "LCD init failed: %d", ret);
        return ret;
    }

    shell_print(sh, "LCD initialized");
    return 0;
}

static int cmd_lcd_status(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    const bool ready = ili9481_support_ready();
    shell_print(sh, "LCD status: %s", ready ? "ready" : "not ready");
    if (!ready) {
        shell_print(sh, "Run: lcd init");
    }

    return 0;
}

static int cmd_lcd_clear(const struct shell* sh, size_t argc, char** argv) {
    if (argc != 2U) {
        shell_error(sh, "Usage: lcd clear <color565>");
        return -EINVAL;
    }

    int ret = lcd_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint16_t color = 0U;
    if (!parse_u16_arg(argv[1], &color)) {
        shell_error(sh, "Invalid color565: %s", argv[1]);
        return -EINVAL;
    }

    ret = ili9481_support_clear(color);
    if (ret != 0) {
        shell_error(sh, "LCD clear failed: %d", ret);
        return ret;
    }

    shell_print(sh, "LCD clear done: color=0x%04x", color);
    return 0;
}

static int cmd_lcd_fill(const struct shell* sh, size_t argc, char** argv) {
    if (argc != 6U) {
        shell_error(sh, "Usage: lcd fill <x> <y> <w> <h> <color565>");
        return -EINVAL;
    }

    int ret = lcd_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint16_t x = 0U;
    uint16_t y = 0U;
    uint16_t w = 0U;
    uint16_t h = 0U;
    uint16_t color = 0U;
    if (!parse_u16_arg(argv[1], &x)) {
        shell_error(sh, "Invalid x: %s", argv[1]);
        return -EINVAL;
    }
    if (!parse_u16_arg(argv[2], &y)) {
        shell_error(sh, "Invalid y: %s", argv[2]);
        return -EINVAL;
    }
    if (!parse_u16_arg(argv[3], &w) || w == 0U) {
        shell_error(sh, "Invalid w: %s", argv[3]);
        return -EINVAL;
    }
    if (!parse_u16_arg(argv[4], &h) || h == 0U) {
        shell_error(sh, "Invalid h: %s", argv[4]);
        return -EINVAL;
    }
    if (!parse_u16_arg(argv[5], &color)) {
        shell_error(sh, "Invalid color565: %s", argv[5]);
        return -EINVAL;
    }

    ret = ili9481_support_fill_rect(x, y, w, h, color);
    if (ret != 0) {
        shell_error(sh, "LCD fill failed: %d", ret);
        return ret;
    }

    shell_print(sh, "LCD fill done: x=%u y=%u w=%u h=%u color=0x%04x", x, y, w, h, color);
    return 0;
}

static int cmd_lcd_test(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    int ret = lcd_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    static const uint16_t colors[] = {
        0xF800U, /* red */
        0x07E0U, /* green */
        0x001FU, /* blue */
        0xFFFFU, /* white */
        0x0000U, /* black */
    };

    shell_print(sh, "LCD test start");
    for (size_t i = 0U; i < (sizeof(colors) / sizeof(colors[0])); ++i) {
        ret = ili9481_support_clear(colors[i]);
        if (ret != 0) {
            shell_error(sh, "LCD test failed at step %u: %d", static_cast<unsigned int>(i), ret);
            return ret;
        }
        k_msleep(250);
    }

    ret = ili9481_support_fill_rect(0U, 0U, 80U, 80U, 0xF800U);
    if (ret != 0) {
        shell_error(sh, "LCD test fill #1 failed: %d", ret);
        return ret;
    }
    ret = ili9481_support_fill_rect(80U, 0U, 80U, 80U, 0x07E0U);
    if (ret != 0) {
        shell_error(sh, "LCD test fill #2 failed: %d", ret);
        return ret;
    }
    ret = ili9481_support_fill_rect(160U, 0U, 80U, 80U, 0x001FU);
    if (ret != 0) {
        shell_error(sh, "LCD test fill #3 failed: %d", ret);
        return ret;
    }

    shell_print(sh, "LCD test done");
    return 0;
}

static int cmd_nor_info(const struct shell* sh, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t size_bytes = 0U;
    uint8_t jedec[3]    = {0U};
    size_t write_block  = w25q64_support_get_write_block_size();

    ret = w25q64_support_get_size(&size_bytes);
    if (ret != 0) {
        shell_error(sh, "NOR get size failed: %d", ret);
        return ret;
    }

    shell_print(sh, "NOR info:");
    shell_print(sh, "  Size: %u bytes (%u KiB)", size_bytes, size_bytes / 1024U);
    shell_print(sh, "  Write block: %u bytes", static_cast<unsigned int>(write_block));
    shell_print(sh, "  Page size: 256 bytes");
    shell_print(sh, "  Sector size: 4096 bytes");

#if defined(CONFIG_FLASH_JESD216_API)
    ret = w25q64_support_read_jedec_id(jedec);
    if (ret == 0) {
        shell_print(sh, "  JEDEC ID: %02x %02x %02x", jedec[0], jedec[1], jedec[2]);
    } else {
        shell_warn(sh, "  JEDEC ID unavailable: %d", ret);
    }
#else
    shell_print(sh, "  JEDEC ID: disabled (CONFIG_FLASH_JESD216_API=n)");
#endif

    return 0;
}

static int cmd_nor_read(const struct shell* sh, size_t argc, char** argv) {
    if (argc < 3U || argc > 4U) {
        shell_error(sh, "Usage: nor read <addr> <len> [dump]");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t addr = 0U;
    size_t len = 0U;
    if (!parse_addr_arg(argv[1], &addr)) {
        shell_error(sh, "Invalid addr: %s", argv[1]);
        return -EINVAL;
    }
    if (!parse_size_arg(argv[2], &len) || len == 0U || len > k_nor_shell_max_read) {
        shell_error(sh, "Invalid len: %s (1..%u)", argv[2], static_cast<unsigned int>(k_nor_shell_max_read));
        return -EINVAL;
    }

    const bool do_dump = (argc == 4U) && (strcmp(argv[3], "dump") == 0);

    uint8_t buffer[k_nor_shell_rw_chunk] = {0U};
    size_t remain = len;
    uint32_t cursor = addr;

    shell_print(sh, "NOR read: addr=0x%08x len=%u", addr, static_cast<unsigned int>(len));

    while (remain > 0U) {
        size_t chunk = (remain > sizeof(buffer)) ? sizeof(buffer) : remain;
        ret = w25q64_support_read(cursor, buffer, chunk);
        if (ret != 0) {
            shell_error(sh, "Read failed at 0x%08x: %d", cursor, ret);
            return ret;
        }

        if (do_dump) {
            size_t offset = 0U;
            while (offset < chunk) {
                size_t line_len = ((chunk - offset) > k_nor_shell_line_bytes)
                                  ? k_nor_shell_line_bytes
                                  : (chunk - offset);
                shell_fprintf(sh, SHELL_INFO, "0x%08x: ", cursor + static_cast<uint32_t>(offset));
                shell_hexdump(sh, &buffer[offset], line_len);
                offset += line_len;
            }
        }

        cursor += static_cast<uint32_t>(chunk);
        remain -= chunk;
    }

    shell_print(sh, "NOR read done");
    return 0;
}

static int cmd_nor_erase_sector(const struct shell* sh, size_t argc, char** argv) {
    if (argc != 2U) {
        shell_error(sh, "Usage: nor erase_sector <addr>");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t addr = 0U;
    if (!parse_addr_arg(argv[1], &addr)) {
        shell_error(sh, "Invalid addr: %s", argv[1]);
        return -EINVAL;
    }

    ret = w25q64_support_erase_sector(addr);
    if (ret != 0) {
        shell_error(sh, "Erase sector failed: %d", ret);
        return ret;
    }

    shell_print(sh, "NOR sector erased at 0x%08x", addr);
    return 0;
}

static int cmd_nor_erase_range(const struct shell* sh, size_t argc, char** argv) {
    if (argc != 3U) {
        shell_error(sh, "Usage: nor erase_range <addr> <len>");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t addr = 0U;
    size_t len = 0U;
    if (!parse_addr_arg(argv[1], &addr)) {
        shell_error(sh, "Invalid addr: %s", argv[1]);
        return -EINVAL;
    }
    if (!parse_size_arg(argv[2], &len) || len == 0U) {
        shell_error(sh, "Invalid len: %s", argv[2]);
        return -EINVAL;
    }

    ret = w25q64_support_erase_range(addr, len);
    if (ret != 0) {
        shell_error(sh, "Erase range failed: %d", ret);
        return ret;
    }

    shell_print(sh, "NOR erase range done: addr=0x%08x len=%u", addr, static_cast<unsigned int>(len));
    return 0;
}

static int cmd_nor_fill(const struct shell* sh, size_t argc, char** argv) {
    if (argc != 4U) {
        shell_error(sh, "Usage: nor fill <addr> <len> <byte>");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t addr = 0U;
    size_t len = 0U;
    uint8_t value = 0U;
    if (!parse_addr_arg(argv[1], &addr)) {
        shell_error(sh, "Invalid addr: %s", argv[1]);
        return -EINVAL;
    }
    if (!parse_size_arg(argv[2], &len) || len == 0U) {
        shell_error(sh, "Invalid len: %s", argv[2]);
        return -EINVAL;
    }
    if (!parse_u8_arg(argv[3], &value)) {
        shell_error(sh, "Invalid byte: %s", argv[3]);
        return -EINVAL;
    }

    uint8_t pattern[k_nor_shell_rw_chunk];
    memset(pattern, value, sizeof(pattern));

    size_t remain = len;
    uint32_t cursor = addr;
    while (remain > 0U) {
        size_t chunk = (remain > sizeof(pattern)) ? sizeof(pattern) : remain;
        ret = w25q64_support_write(cursor, pattern, chunk);
        if (ret != 0) {
            shell_error(sh, "Fill failed at 0x%08x: %d", cursor, ret);
            return ret;
        }
        cursor += static_cast<uint32_t>(chunk);
        remain -= chunk;
    }

    shell_print(sh, "NOR fill done: addr=0x%08x len=%u value=0x%02x", addr, static_cast<unsigned int>(len), value);
    return 0;
}

static int cmd_nor_write(const struct shell* sh, size_t argc, char** argv) {
    if (argc < 3U) {
        shell_error(sh, "Usage: nor write <addr> <byte0> [byte1 ... byteN]");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t addr = 0U;
    if (!parse_addr_arg(argv[1], &addr)) {
        shell_error(sh, "Invalid addr: %s", argv[1]);
        return -EINVAL;
    }

    const size_t data_len = argc - 2U;
    if (data_len > k_nor_shell_rw_chunk) {
        shell_error(sh, "Too many bytes: %u (max %u)", static_cast<unsigned int>(data_len),
                    static_cast<unsigned int>(k_nor_shell_rw_chunk));
        return -EINVAL;
    }

    uint8_t data[k_nor_shell_rw_chunk] = {0U};
    for (size_t i = 0U; i < data_len; ++i) {
        if (!parse_u8_arg(argv[i + 2U], &data[i])) {
            shell_error(sh, "Invalid byte[%u]: %s", static_cast<unsigned int>(i), argv[i + 2U]);
            return -EINVAL;
        }
    }

    ret = w25q64_support_write(addr, data, data_len);
    if (ret != 0) {
        shell_error(sh, "Write failed: %d", ret);
        return ret;
    }

    uint8_t verify[k_nor_shell_rw_chunk] = {0U};
    ret = w25q64_support_read(addr, verify, data_len);
    if (ret != 0) {
        shell_warn(sh, "Readback failed: %d", ret);
        return ret;
    }

    if (memcmp(data, verify, data_len) != 0) {
        shell_error(sh, "Write verify mismatch");
        return -EIO;
    }

    shell_print(sh, "NOR write+verify done: addr=0x%08x len=%u", addr, static_cast<unsigned int>(data_len));
    return 0;
}

static int cmd_nor_verify(const struct shell* sh, size_t argc, char** argv) {
    if (argc < 3U) {
        shell_error(sh, "Usage: nor verify <addr> <byte0> [byte1 ... byteN]");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t addr = 0U;
    if (!parse_addr_arg(argv[1], &addr)) {
        shell_error(sh, "Invalid addr: %s", argv[1]);
        return -EINVAL;
    }

    const size_t data_len = argc - 2U;
    if (data_len > k_nor_shell_rw_chunk) {
        shell_error(sh, "Too many bytes: %u (max %u)", static_cast<unsigned int>(data_len),
                    static_cast<unsigned int>(k_nor_shell_rw_chunk));
        return -EINVAL;
    }

    uint8_t expected[k_nor_shell_rw_chunk] = {0U};
    for (size_t i = 0U; i < data_len; ++i) {
        if (!parse_u8_arg(argv[i + 2U], &expected[i])) {
            shell_error(sh, "Invalid byte[%u]: %s", static_cast<unsigned int>(i), argv[i + 2U]);
            return -EINVAL;
        }
    }

    uint8_t actual[k_nor_shell_rw_chunk] = {0U};
    ret = w25q64_support_read(addr, actual, data_len);
    if (ret != 0) {
        shell_error(sh, "Read failed: %d", ret);
        return ret;
    }

    if (memcmp(expected, actual, data_len) != 0) {
        shell_error(sh, "Verify failed at addr=0x%08x", addr);
        shell_print(sh, "Expected:");
        shell_hexdump(sh, expected, data_len);
        shell_print(sh, "Actual:");
        shell_hexdump(sh, actual, data_len);
        return -EIO;
    }

    shell_print(sh, "NOR verify pass: addr=0x%08x len=%u", addr, static_cast<unsigned int>(data_len));
    return 0;
}

static int cmd_nor_test(const struct shell* sh, size_t argc, char** argv) {
    if (argc != 2U) {
        shell_error(sh, "Usage: nor test <sector_addr>");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t sector_addr = 0U;
    if (!parse_addr_arg(argv[1], &sector_addr)) {
        shell_error(sh, "Invalid sector_addr: %s", argv[1]);
        return -EINVAL;
    }

    uint8_t tx[k_nor_shell_rw_chunk] = {0U};
    uint8_t rx[k_nor_shell_rw_chunk] = {0U};
    for (size_t i = 0U; i < sizeof(tx); ++i) {
        tx[i] = static_cast<uint8_t>((i ^ 0x5aU) & 0xffU);
    }

    ret = w25q64_support_erase_sector(sector_addr);
    if (ret != 0) {
        shell_error(sh, "Test erase failed: %d", ret);
        return ret;
    }

    ret = w25q64_support_write(sector_addr, tx, sizeof(tx));
    if (ret != 0) {
        shell_error(sh, "Test write failed: %d", ret);
        return ret;
    }

    ret = w25q64_support_read(sector_addr, rx, sizeof(rx));
    if (ret != 0) {
        shell_error(sh, "Test read failed: %d", ret);
        return ret;
    }

    if (memcmp(tx, rx, sizeof(tx)) != 0) {
        shell_error(sh, "Test mismatch");
        return -EIO;
    }

    shell_print(sh, "NOR R/W test pass at sector 0x%08x (%u bytes)", sector_addr,
                static_cast<unsigned int>(sizeof(tx)));
    return 0;
}

/*-------- 5. shell command definitions ------------------------------------------------------------------------------*/

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_signal, SHELL_CMD(start, NULL, "Start signal output", cmd_signal_start),
    SHELL_CMD(stop, NULL, "Stop signal output", cmd_signal_stop),
    SHELL_CMD(pause, NULL, "Pause signal output", cmd_signal_pause),
    SHELL_CMD(resume, NULL, "Resume signal output", cmd_signal_resume),
    SHELL_CMD(freq, NULL, "Set frequency (Hz)", cmd_signal_freq),
    SHELL_CMD(amp, NULL, "Set amplitude (mV)", cmd_signal_amp),
    SHELL_CMD(status, NULL, "Show signal status", cmd_signal_status), SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_lcd,
    SHELL_CMD(init, NULL, "Initialize ILI9481 panel", cmd_lcd_init),
    SHELL_CMD(status, NULL, "Show LCD init status", cmd_lcd_status),
    SHELL_CMD(clear, NULL, "Clear screen: lcd clear <color565>", cmd_lcd_clear),
    SHELL_CMD(fill, NULL, "Fill rect: lcd fill <x> <y> <w> <h> <color565>", cmd_lcd_fill),
    SHELL_CMD(test, NULL, "Run LCD color and block test", cmd_lcd_test),
    SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_nor,
    SHELL_CMD(probe, NULL, "Probe/initialize NOR device", cmd_nor_probe),
    SHELL_CMD(info, NULL, "Show NOR capacity and JEDEC info", cmd_nor_info),
    SHELL_CMD(read, NULL, "Read NOR: nor read <addr> <len> [dump]", cmd_nor_read),
    SHELL_CMD(write, NULL, "Write bytes: nor write <addr> <b0> [b1 ...]", cmd_nor_write),
    SHELL_CMD(verify, NULL, "Verify bytes: nor verify <addr> <b0> [b1 ...]", cmd_nor_verify),
    SHELL_CMD(fill, NULL, "Fill bytes: nor fill <addr> <len> <byte>", cmd_nor_fill),
    SHELL_CMD(erase_sector, NULL, "Erase 4KB sector: nor erase_sector <addr>", cmd_nor_erase_sector),
    SHELL_CMD(erase_range, NULL, "Erase range: nor erase_range <addr> <len>", cmd_nor_erase_range),
    SHELL_CMD(test, NULL, "R/W test: nor test <sector_addr>", cmd_nor_test),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(signal, &sub_signal, "Signal generator commands", NULL);
SHELL_CMD_REGISTER(lcd, &sub_lcd, "ILI9481 LCD commands", NULL);
SHELL_CMD_REGISTER(nor, &sub_nor, "NOR flash commands", NULL);
SHELL_CMD_REGISTER(sysres, NULL,
                   "Show resource usage: sysres | sysres reset | sysres watch <period_ms> [count]", cmd_sysres);

/*-------- 6. public functions ---------------------------------------------------------------------------------------*/

void register_signal_shell_commands(CommandBusPort& command_bus, RequestBusPort& request_bus) {
    g_command_bus = &command_bus;
    g_request_bus = &request_bus;
    LOG_INF("Signal shell commands registered");
}

} // namespace omnigen
