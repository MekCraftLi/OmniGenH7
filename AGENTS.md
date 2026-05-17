# Repository Guidelines

## Project Structure & Module Organization
- `src/` contains application code:
  - `src/app/`: startup and dependency wiring (`main.cpp`, composition root).
  - `src/domain/`: signal entities, commands, validation, synthesis logic.
  - `src/services/`: orchestration (`SignalEngine`).
  - `src/platform/`: Zephyr and mock port adapters.
  - `src/diagnostics/`: Zephyr shell diagnostics/CLI commands.
- Headers live next to their implementation modules under `src/`; C driver headers live under `drivers/include/`.
- `drivers/` holds board-level C drivers (DAC output, W25Q64 storage).
- `boards/st/omnigen_h7/` contains DTS/Kconfig/board config.
- `dts/bindings/` stores custom Devicetree bindings.
- `docs/` contains architecture notes, handover docs, and templates.

## Build, Test, and Development Commands
- `west build -b omnigen_h7 . -- -DBOARD_ROOT=.`: default build.
- `west build -b omnigen_h7 . -p always -- -DBOARD_ROOT=.`: pristine rebuild.
- `west flash --runner stm32cubeprogrammer` (or `openocd`, `jlink`): flash firmware.
- `west build -t menuconfig`: inspect/update Zephyr config interactively.
- `west build -t clean`: clean build artifacts for current build directory.

## Coding Style & Naming Conventions
- Language mix is C++ (core app) + C (low-level drivers).
- Format with `.clang-format` (LLVM-based): 4-space indent, 120-column limit, left pointer alignment, attached braces.
- Use `snake_case` for files/functions/variables; keep module names aligned within `src/*`.
- All C/C++ files use the project Doxygen file banner (`@file`, `@brief`, `@attention`, `@note`, `@author`, `@date`, `@version`).
- Organize file bodies with numbered section dividers (`1. includes and imports`, `2. enum and define`, `3. interface`/`implementation`).
- Keep platform-specific code in `platform/` or `drivers/`; keep domain logic hardware-agnostic.

## Testing Guidelines
- No dedicated `tests/` suite is committed yet; use build + on-target smoke tests.
- Minimum validation before PR:
  - Build succeeds with pristine config.
  - Flash succeeds on OmniGen H7 board.
  - Shell command flows behave as expected (examples: `signal start`, `signal status`, `sysres`).
- When adding automated tests, prefer Zephyr `ztest` and place under a new `tests/` tree.

## Commit & Pull Request Guidelines
- Follow current history style: Conventional Commits with optional scope, e.g. `feat(storage): ...`, `docs: ...`, `feat(phase4): ...`.
- Keep commits focused by subsystem (domain/service/platform/driver/board/docs).
- PRs should include: change summary, touched board/config files (`prj.conf`, DTS, Kconfig), test evidence (build/flash/shell output), and hardware assumptions.
