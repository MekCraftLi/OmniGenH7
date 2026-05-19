/**
 *******************************************************************************
 * @file    sysres_shell_commands.cpp
 * @brief   System resource shell diagnostics
 *******************************************************************************
 * @attention
 *
 * Provides the `sysres` shell command for CPU, heap, and thread resource
 * snapshots.
 *
 *******************************************************************************
 * @note
 *
 * This module owns only diagnostic counters used to report peak values within
 * the current sampling window. It does not own kernel resources.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/19
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "diagnostics/shell_parse.hpp"

#include <errno.h>
#include <string.h>
#include <zephyr/debug/cpu_load.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/mem_stats.h>
#include <zephyr/sys/sys_heap.h>

#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS) && (K_HEAP_MEM_POOL_SIZE > 0)
extern "C" struct k_heap _system_heap;
#endif

namespace omnigen {

/*-------- 2. data structures ----------------------------------------------------------------------------------------*/

/**
 * @brief 系统资源线程遍历上下文。
 *
 * Shell 资源诊断命令遍历 Zephyr 线程列表时使用该结构保存输出目标、线程计数和可选的
 * 运行时统计基准。该结构不拥有线程对象，所有线程生命周期由 Zephyr 内核管理。
 */
struct SysResThreadDumpContext {
    const struct shell* sh{nullptr}; /**< Shell 输出目标，不由本结构拥有。 */
    uint32_t thread_count{0U};       /**< 本次遍历已经输出的线程数量。 */
#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
    k_thread_runtime_stats_t all_stats{}; /**< 全局运行时统计基准。 */
    bool runtime_stats_ready{false};       /**< 全局运行时统计是否可用于计算线程 CPU 占比。 */
#endif
};

/**
 * @brief 系统资源快照结构体。
 *
 * 保存一次 `sysres` 诊断采样得到的运行时间、CPU 负载、堆统计和线程数量等信息。条件编译
 * 字段只在对应 Zephyr 功能启用时存在，快照本身为值对象，不引用内核资源。
 */
struct SysResSnapshot {
    uint32_t uptime_ms{0U}; /**< 系统运行时间，单位 ms。 */
#if defined(CONFIG_CPU_LOAD)
    int cpu_pm{-ENOTSUP}; /**< CPU 负载，单位为千分之一；负值表示不可用。 */
#endif
#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
    bool scheduler_active_available{false}; /**< 调度器活动统计是否可用。 */
    uint32_t scheduler_active_pm{0U};        /**< 调度器活动比例，单位为千分之一。 */
#endif
#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS) && (K_HEAP_MEM_POOL_SIZE > 0)
    int heap_rc{-ENOTSUP};                  /**< 堆统计读取结果。 */
    struct sys_memory_stats heap_stats{};   /**< Zephyr 系统堆统计值。 */
#endif
    uint32_t thread_count{0U}; /**< 当前线程数量。 */
};

/*-------- 3. variables ----------------------------------------------------------------------------------------------*/

static uint32_t g_sysres_sample_count = 0U;
#if defined(CONFIG_CPU_LOAD)
static int g_sysres_peak_cpu_pm = -ENOTSUP;
#endif
#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
static uint32_t g_sysres_peak_sched_pm = 0U;
#endif
static uint32_t g_sysres_peak_thread_count = 0U;

/*-------- 4. internal helpers ---------------------------------------------------------------------------------------*/

static uint32_t ratio_to_permille_u64(uint64_t numerator, uint64_t denominator)
{
    if (denominator == 0ULL) {
        return 0U;
    }

    uint64_t scaled = (numerator * 1000ULL) / denominator;
    if (scaled > 1000ULL) {
        scaled = 1000ULL;
    }

    return static_cast<uint32_t>(scaled);
}

static uint32_t thread_cpu_permille(const SysResThreadDumpContext& ctx, k_tid_t tid)
{
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

static void dump_thread_resource(const struct k_thread* cthread, void* user_data)
{
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
    const char* state   = k_thread_state_str(thread, state_buf, sizeof(state_buf));
    const int priority  = k_thread_priority_get(thread);
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

static void sysres_reset_window_and_peaks(void)
{
    g_sysres_sample_count      = 0U;
    g_sysres_peak_thread_count = 0U;

#if defined(CONFIG_CPU_LOAD)
    (void)cpu_load_get(true);
    g_sysres_peak_cpu_pm = -ENOTSUP;
#endif

#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
    g_sysres_peak_sched_pm = 0U;
#endif
}

static SysResSnapshot collect_sysres_snapshot(void)
{
    SysResSnapshot snapshot{};
    snapshot.uptime_ms = k_uptime_get_32();

#if defined(CONFIG_CPU_LOAD)
    snapshot.cpu_pm = cpu_load_get(false);
#endif

    SysResThreadDumpContext thread_ctx{};
#if defined(CONFIG_THREAD_RUNTIME_STATS) && defined(CONFIG_SCHED_THREAD_USAGE)
    if (k_thread_runtime_stats_all_get(&thread_ctx.all_stats) == 0) {
        thread_ctx.runtime_stats_ready      = true;
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

static void print_sysres_snapshot(const struct shell* sh, bool include_thread_table)
{
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

/*-------- 5. command handlers ---------------------------------------------------------------------------------------*/

static int cmd_sysres(const struct shell* sh, size_t argc, char** argv)
{
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
        if (!parse_shell_u32_arg(argv[2], &period_ms) || period_ms == 0U) {
            shell_error(sh, "Invalid period_ms: %s", argv[2]);
            return -EINVAL;
        }

        uint32_t count = 10U;
        if (argc == 4U) {
            if (!parse_shell_u32_arg(argv[3], &count) || count == 0U) {
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

/*-------- 6. shell command definitions ------------------------------------------------------------------------------*/

SHELL_CMD_REGISTER(sysres, NULL,
                   "Show resource usage: sysres | sysres reset | sysres watch <period_ms> [count]", cmd_sysres);

} // namespace omnigen
