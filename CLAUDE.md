# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OmniGen H7 is a Zephyr RTOS board definition project for a custom STM32H723ZG-based board. The current application implements a DMA-based UART echo system using USART3 with idle line detection.

## Build Commands

```bash
# Build the project
west build -b omnigen_h7 . -- -DBOARD_ROOT=.

# Clean build
west build -b omnigen_h7 . -p always -- -DBOARD_ROOT=.

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

The application uses STM32 Low-Level (LL) drivers directly for DMA and UART idle interrupt handling, bypassing Zephyr's async UART API. This is necessary because the Zephyr STM32 UART driver does not expose the idle line detection feature.

Key implementation details:
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

## Zephyr Submodule

The `zephyr/` directory is a git submodule. The actual Zephyr base used for builds is typically in a west workspace (e.g., `~/zephyrproject/zephyr`).
