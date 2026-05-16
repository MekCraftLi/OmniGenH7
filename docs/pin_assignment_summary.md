# OmniGen H7 计划使用引脚汇总

日期：2026-05-15

本文件只记录项目计划实际使用的 MCU 引脚，不包含扩展保留引脚。

| 序号 | 模块 | 功能 | 引脚 | 方向 | 复用/配置 | 备注 |
|---:|---|---|---:|---|---|---|
| 1 | DAC 输出 | 模拟波形输出 | PA4 | 模拟输出 | DAC1_OUT1 | 主波形输出 |
| 2 | LCD | ILI9481 复位 | PA8 | GPIO 输出 | 低电平有效 | LCD reset |
| 3 | 串口控制台 | USART1 TX | PA9 | 复用输出 | USART1_TX, AF7 | Zephyr console / shell |
| 4 | 串口控制台 | USART1 RX | PA10 | 复用输入 | USART1_RX, AF7 | Zephyr console / shell |
| 5 | 用户 LED | 板载 LED | PG7 | GPIO 输出 | 高电平点亮 | 已定义为 `led0` |
| 6 | LCD FMC | 片选 CS | PC7 | 复用输出 | FMC_NE1, AF9 | FMC Bank1 |
| 7 | LCD FMC | 读信号 RD | PD4 | 复用输出 | FMC_NOE, AF12 | 8080 并口读控制 |
| 8 | LCD FMC | 写信号 WR | PD5 | 复用输出 | FMC_NWE, AF12 | 8080 并口写控制 |
| 9 | LCD FMC | 数据/命令选择 RS/DCX | PD11 | 复用输出 | FMC_A16, AF12 | 新版硬件使用 A16，数据地址 `0x60020000` |
| 10 | LCD FMC | 数据 D0 | PD14 | 复用 I/O | FMC_D0, AF12 | 16-bit 数据总线 |
| 11 | LCD FMC | 数据 D1 | PD15 | 复用 I/O | FMC_D1, AF12 | 16-bit 数据总线 |
| 12 | LCD FMC | 数据 D2 | PD0 | 复用 I/O | FMC_D2, AF12 | 16-bit 数据总线 |
| 13 | LCD FMC | 数据 D3 | PD1 | 复用 I/O | FMC_D3, AF12 | 16-bit 数据总线 |
| 14 | LCD FMC | 数据 D4 | PE7 | 复用 I/O | FMC_D4, AF12 | 16-bit 数据总线 |
| 15 | LCD FMC | 数据 D5 | PE8 | 复用 I/O | FMC_D5, AF12 | 16-bit 数据总线 |
| 16 | LCD FMC | 数据 D6 | PE9 | 复用 I/O | FMC_D6, AF12 | 16-bit 数据总线 |
| 17 | LCD FMC | 数据 D7 | PE10 | 复用 I/O | FMC_D7, AF12 | 16-bit 数据总线 |
| 18 | LCD FMC | 数据 D8 | PE11 | 复用 I/O | FMC_D8, AF12 | 16-bit 数据总线 |
| 19 | LCD FMC | 数据 D9 | PE12 | 复用 I/O | FMC_D9, AF12 | 16-bit 数据总线 |
| 20 | LCD FMC | 数据 D10 | PE13 | 复用 I/O | FMC_D10, AF12 | 16-bit 数据总线 |
| 21 | LCD FMC | 数据 D11 | PE14 | 复用 I/O | FMC_D11, AF12 | 16-bit 数据总线 |
| 22 | LCD FMC | 数据 D12 | PE15 | 复用 I/O | FMC_D12, AF12 | 16-bit 数据总线 |
| 23 | LCD FMC | 数据 D13 | PD8 | 复用 I/O | FMC_D13, AF12 | 16-bit 数据总线 |
| 24 | LCD FMC | 数据 D14 | PD9 | 复用 I/O | FMC_D14, AF12 | 16-bit 数据总线 |
| 25 | LCD FMC | 数据 D15 | PD10 | 复用 I/O | FMC_D15, AF12 | 16-bit 数据总线 |
| 26 | NOR Flash | OSPI IO0 | PF8 | 复用 I/O | OCTOSPIM_P1_IO0, AF10 | W25Q64 |
| 27 | NOR Flash | OSPI IO1 | PF9 | 复用 I/O | OCTOSPIM_P1_IO1, AF10 | W25Q64 |
| 28 | NOR Flash | OSPI IO2 | PF7 | 复用 I/O | OCTOSPIM_P1_IO2, AF10 | W25Q64 |
| 29 | NOR Flash | OSPI IO3 | PF6 | 复用 I/O | OCTOSPIM_P1_IO3, AF10 | W25Q64 |
| 30 | NOR Flash | OSPI CLK | PF10 | 复用输出 | OCTOSPIM_P1_CLK, AF9 | W25Q64 时钟 |
| 31 | NOR Flash | OSPI NCS | PG6 | 复用输出 | OCTOSPIM_P1_NCS, AF10 | W25Q64 片选 |
| 32 | 滤波器切换 | FILTER_SEL0 | PB0 | GPIO 输出 | 推挽输出 | 模拟开关选择位 0 |
| 33 | 滤波器切换 | FILTER_SEL1 | PB1 | GPIO 输出 | 推挽输出 | 模拟开关选择位 1 |
| 34 | 滤波器切换 | FILTER_EN | PB2 | GPIO 输出 | 推挽输出 | 模拟开关使能 |
| 35 | 输出控制 | OUTPUT_MUTE | PB10 | GPIO 输出 | 推挽输出 | 切换滤波器时静音/断开输出 |
| 36 | 主编码器 | ENC_MAIN_A | PB3 | GPIO 输入 | 上拉输入，中断 | 主编码器 A 相 |
| 37 | 主编码器 | ENC_MAIN_B | PB4 | GPIO 输入 | 上拉输入，中断 | 主编码器 B 相 |
| 38 | 主编码器 | ENC_MAIN_SW | PB5 | GPIO 输入 | 上拉输入，消抖 | 主编码器按键 |
| 39 | 副编码器 | ENC_FINE_A | PB6 | GPIO 输入 | 上拉输入，中断 | 副编码器 A 相 |
| 40 | 副编码器 | ENC_FINE_B | PB7 | GPIO 输入 | 上拉输入，中断 | 副编码器 B 相 |
| 41 | 副编码器 | ENC_FINE_SW | PB8 | GPIO 输入 | 上拉输入，消抖 | 副编码器按键 |
| 42 | 按键 | BTN_RUN | PB9 | GPIO 输入 | 上拉输入，消抖 | 启动/停止输出 |
| 43 | 按键 | BTN_WAVE | PB11 | GPIO 输入 | 上拉输入，消抖 | 波形选择 |
| 44 | 按键 | BTN_FILTER | PB12 | GPIO 输入 | 上拉输入，消抖 | 滤波器档位选择 |
| 45 | 按键 | BTN_BACK | PB13 | GPIO 输入 | 上拉输入，消抖 | 返回/取消 |
| 46 | 按键 | BTN_STORE | PC0 | GPIO 输入 | 上拉输入，消抖 | 保存预设/用户功能 |
| 47 | 按键 | BTN_MENU | PC1 | GPIO 输入 | 上拉输入，消抖 | 菜单/确认 |
| 48 | WS2812 | WS2812_DATA | PC6 | GPIO/PWM 输出 | GPIO 或 TIM3_CH1/TIM8_CH1 | 1 根数据线，计划 4~8 颗灯珠 |
| 49 | 状态 LED | LED_STATUS | PC8 | GPIO 输出 | 推挽输出 | 系统状态指示 |
| 50 | 状态 LED | LED_FAULT | PC9 | GPIO 输出 | 推挽输出 | 故障状态指示 |
| 51 | 蜂鸣器 | BUZZER_PWM | PB15 | PWM 输出 | TIM12_CH2, AF2 | 无源蜂鸣器 PWM；有源蜂鸣器可当 GPIO 使用 |

