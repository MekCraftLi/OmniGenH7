# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OmniGen H7 is a Zephyr RTOS board definition project for a custom STM32H723ZG-based board. The current application implements a DMA-based UART echo system using USART3 with idle line detection.

## Build Commands

```bash
# Build commands must run inside the conda zephyr environment.
# In non-interactive PowerShell, prefer `conda run -n zephyr ...` because `conda activate`
# may require shell initialization.
conda activate zephyr

# Build the project
west build -b omnigen_h7 . -- -DBOARD_ROOT=.
# Non-interactive PowerShell equivalent:
conda run -n zephyr west build -b omnigen_h7 . -- -DBOARD_ROOT=C:/env/zephyrproject/OmniGenH7

# Clean build
west build -b omnigen_h7 . -p always -- -DBOARD_ROOT=.
# Non-interactive PowerShell equivalent:
conda run -n zephyr west build -b omnigen_h7 . -p always -- -DBOARD_ROOT=C:/env/zephyrproject/OmniGenH7

# Flash the board (choose one)
west flash --runner stm32cubeprogrammer  # STM32CubeProgrammer via SWD
west flash --runner openocd              # OpenOCD with CMSIS-DAP
west flash --runner jlink                # J-Link

# Open serial console (USART3, 115200 baud)
minicom -D /dev/ttyUSB0 -b 115200
```

## Hardware Configuration

- **MCU**: STM32H723ZG (ARM Cortex-M7, 550MHz)
- **Console**: USART3 (PD8=TX, PD9=RX) at 115200 baud
- **User LED**: GPIOG pin 7 (active high)
- **Debug**: SWD interface
- **DMA**: DMA1 Stream 5 (RX), DMA1 Stream 4 (TX) for USART3

## Architecture

The application follows Clean Architecture / Ports & Adapters pattern with a unified data bus design.

### Ports & Adapters

- `src/ports/` — Abstract interfaces (ports) for hardware capabilities
  - `WaveSinkPort`, `StoragePort`, `DisplayPort`, `FilterSwitchPort`
  - `CommandBusPort` — write-side command dispatch
  - `RequestBusPort` — read-side query dispatch
- `src/platform/` — Concrete Zephyr adapters implementing the ports
- `src/services/` — Domain services (`SignalEngine`, `DirectCommandBus`, `DirectRequestBus`)
- `src/domain/` — Pure domain types and algorithms
- `src/app/composition_root.cpp` — `SystemContext` owns all instances, wires dependencies
- `drivers/` — C-level hardware driver support (DAC, ILI9481, W25Q64)

### Data Bus & Naming Convention

All data exchanged across module boundaries follows a strict taxonomy:

| Category | Meaning | Examples |
|----------|---------|----------|
| **Command** | Write intent (do something) | `SignalCommand`, `FilterCommand`, `AppCommand` |
| **Request** | Read/query intent | `AppRequest`, `StorageReadRequest`, `DisplayBlitRequest` |
| **Response** | Read/query result envelope | `AppResponse` |
| **Status** | Current module state (small) | `FilterStatus`, `StorageStatus`, `DisplayStatus`, `SystemStatus` |
| **Snapshot** | Detailed service state copy (large) | `SignalEngineSnapshot` |
| **Payload/Block** | Bulk data descriptor (pass by ref) | `WaveSampleBlock` |

Flow:
- **Write path**: Shell/UI → `AppCommand` → `CommandBusPort::submit()` → `DirectCommandBus` → service
- **Read path**: Shell/UI → `AppRequest` → `RequestBusPort::request()` → `DirectRequestBus` → `AppResponse`
- Status structs are simple aggregates (no defaults) to support designated initializers.
- `AppCommand` carries metadata: `source` (who sent it) and `sequence` (monotonic ID).

### Key Implementation Details

- **DMA RX**: Uses LL DMA in normal mode with idle line interrupt to detect packet boundaries
- **DMA TX**: Configured per-transfer, waits for TC (transfer complete) flag
- **ISR**: `uart_isr_handler` clears IDLE flag and calculates received length from DMA CNDTR

## Code Style

Follow Zephyr RTOS coding conventions (Linux kernel style with Zephyr-specific rules):
- Tabs: 8 characters; line length ≤ 100 columns
- `snake_case` for all identifiers
- Braces required on all control statements, even single-line blocks
- Use `/* */` comments (not `//`)
- SPDX license headers required on all new files
- All C/C++ source and header files must use the project Doxygen file banner style:
  `@file`, `@brief`, `@attention`, `@note`, `@author`, `@date`, `@version`
- Organize file bodies with numbered section dividers like `/*-------- 1. includes and imports ----*/`, `2. enum and define`, `3. interface` or `3. implementation`

## Zephyr Submodule

The `zephyr/` directory is a git submodule. The actual Zephyr base used for builds is typically in a west workspace (e.g., `~/zephyrproject/zephyr`).

## Signal Simulation

MATLAB installation: `C:\Program Files\MATLAB\R2024b\bin\matlab.exe`

Simulation folder: `simulation/matlab/`
- `waveform_generator.m` — Core waveform synthesis (matches firmware C++ algorithms exactly)
- `dac_sim.m` — DAC output simulation: configure WAVEFORM/FREQ_HZ/AMPL_MV/OFFSET_MV and run
- `dac_dual_sine.m` — Compare 100kHz vs 1kHz sine output

To run simulation (PowerShell):
```powershell
& "C:\Program Files\MATLAB\R2024b\bin\matlab.exe" -nosplash -r "cd('simulation\matlab'); dac_sim;"
```

Firmware waveform algorithm locations for cross-reference:
- `src/domain/waveform_synthesis.cpp` — Sine/Sq/Tri/Saw generation (LUT-based)
- `src/domain/waveform_synthesis.hpp` — Public API
- `src/domain/signal_profile.hpp` — Profile structs (WaveformKind, SignalLimits)
