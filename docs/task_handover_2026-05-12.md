# Task Handover (2026-05-12)

## Current Repository State
- Branch: `dev`
- HEAD: `2d5ad7c`
- Working tree: clean except local IDE folder `/.idea` (untracked, not committed).

## What Was Completed
1. Added system resource shell command:
   - New command: `sysres`
   - File: `src/diagnostics/shell_commands.cpp`
2. Enabled runtime stats needed by `sysres`:
   - File: `prj.conf`
   - Added:
     - `CONFIG_THREAD_MONITOR=y`
     - `CONFIG_THREAD_RUNTIME_STATS=y`
     - `CONFIG_SCHED_THREAD_USAGE=y`
     - `CONFIG_SCHED_THREAD_USAGE_ALL=y`
     - `CONFIG_CPU_LOAD=y`
     - `CONFIG_CPU_LOAD_LOG_PERIODICALLY=0`
     - `CONFIG_SYS_HEAP_RUNTIME_STATS=y`
     - `CONFIG_HEAP_MEM_POOL_SIZE=16384`
3. Fixed heap symbol linkage in C++:
   - `extern "C" struct k_heap _system_heap;`

## `sysres` Output Scope
- Uptime
- CPU load
- Scheduler active ratio (from thread runtime stats)
- Heap usage (allocated/free/max/total)
- Thread table:
  - name, tid, priority, state, cpu%, stack usage
  - total thread count

## Build/Validation Done
- Build command:
  - `west build -b omnigen_h7/stm32h723xx . -- -DBOARD_ROOT=.`
- Result:
  - Build success on `dev` after merge/conflict resolution.

## Relevant Commits
- `2d5ad7c` (`dev`, `origin/dev`): add `sysres` command and resource stats config.
- `f583e84`: phase4 DAC output driver and waveform synthesis baseline.
- Older integration commit from feature line: `91a24af` (not on `dev`, for reference only).

## Known Notes / Risks
- `sysres` heap line is available only when both:
  - `CONFIG_SYS_HEAP_RUNTIME_STATS=y`
  - `CONFIG_HEAP_MEM_POOL_SIZE > 0` (already set to `16384`).
- CPU load statistics are runtime-window based; first readings right after boot can be unstable.
- There are existing warnings in other files (e.g. `drivers/output/dac_wave_sink.c` unused symbols) not introduced by this task.

## Suggested Next Steps
1. Flash and run on target, verify:
   - `sysres`
   - `signal start/stop/status`
2. Optional enhancement:
   - Add `sysres reset` (reset CPU-load window / peak stats).
   - Add `sysres watch <ms>` periodic reporting.

