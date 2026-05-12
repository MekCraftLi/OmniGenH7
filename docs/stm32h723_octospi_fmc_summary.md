# STM32H723 OctoSPI / FMC 参考手册要点汇总

本文基于本地参考手册目录 `D:\开发资料\ST官方\stm32h723 reference` 的分段 Markdown 文件整理，面向项目落地使用，不替代原 RM0468。

## 1. 章节定位

### FMC（第 24 章）
- `stm32h723参考手册_pages_821-830.md`
  - 24 FMC 概述、主特性、总线接口
- `stm32h723参考手册_pages_831-840.md`
  - 24.6 外设地址映射
  - 24.7 NOR/PSRAM 控制器（含接口信号和支持事务）
- `stm32h723参考手册_pages_841-850.md`
  - 24.7.3/24.7.4 时序规则与异步事务
- `stm32h723参考手册_pages_861-870.md`
  - 24.7.5 同步事务
  - 24.7.6 NOR/PSRAM 寄存器（FMC_BCRx 等）
- `stm32h723参考手册_pages_871-880.md`
  - FMC_BTRx/FMC_BWTRx
  - 24.8 NAND 控制器（24.8.1~24.8.5）
- `stm32h723参考手册_pages_881-890.md`
  - 24.8.6/24.8.7 NAND ECC 与寄存器
  - 24.9 SDRAM 控制器
- `stm32h723参考手册_pages_901-910.md`
  - 24.9.6 FMC 寄存器总表

### OctoSPI（第 25 章）+ OCTOSPIM（第 26 章）+ DLYB（第 27 章相关）
- `stm32h723参考手册_pages_901-910.md`
  - 25.1~25.4.1（OctoSPI 概述、特性、框图）
- `stm32h723参考手册_pages_911-920.md`
  - 25.4.2~25.4.5（Regular-command / HyperBus 协议）
- `stm32h723参考手册_pages_921-930.md`
  - 25.4.6~25.4.14（模式、配置流程）
- `stm32h723参考手册_pages_931-940.md`
  - 25.4.15~25.7.2（错误管理、中断、核心寄存器）
- `stm32h723参考手册_pages_941-950.md`
  - 25.7.3~25.7.15（DCR/SR/FCR/CCR/TCR 等）
- `stm32h723参考手册_pages_951-960.md`
  - 25.7.16~25.7.28（IR/ABR/LPTR/WP*/W*/HLCR/寄存器表）
- `stm32h723参考手册_pages_961-970.md`
  - 26 OCTOSPIM（IO 管理与复用）
  - 27 DLYB 概述（与 OctoSPI 采样相关）
- `stm32h723参考手册_pages_971-980.md`
  - DLYB 详细配置流程与寄存器

## 2. FMC 关键结论（项目相关）

## 2.1 地址空间与 Bank 映射
- 默认 FMC 外部地址窗口（重点）：
  - `0x6000_0000 ~ 0x6FFF_FFFF`：NOR/PSRAM bank
  - `0x8000_0000 ~ 0x8FFF_FFFF`：NAND bank
  - `0xC000_0000 ~ 0xCFFF_FFFF`：SDRAM bank1
  - `0xD000_0000 ~ 0xDFFF_FFFF`：SDRAM bank2
- 可通过 `FMC_BCR1.BMAP[1:0]` 进行 NOR/PSRAM 与 SDRAM bank1 交换映射。

## 2.2 NOR/PSRAM
- Bank1 拆分为 4 个子 bank（4 路 CS）。
- 控制主寄存器：
  - `FMC_BCRx`：存储器类型、数据宽度、Burst/Wait、扩展模式等。
  - `FMC_BTRx`：读时序。
  - `FMC_BWTRx`：写时序（EXTMOD 使能时）。
- 重要位：
  - `FMC_BCR1.FMCEN`：FMC 总使能（仅 BCR1 有效）。
  - `CCLKEN/CLKDIV/DATLAT`：同步模式关键参数。
  - `BMAP`：地址重映射。
- 同步模式下 `FMC_CLK` 与 `fmc_ker_ck` 存在分频关系，32-bit MWID 时还会受 AXI 访问宽度影响。

## 2.3 NAND
- 使用 Bank3，Common / Attribute 空间分别对应 `FMC_PMEM` / `FMC_PATT` 时序。
- 典型映射：
  - Common：`0x8000_0000 ~ 0x83FF_FFFF`
  - Attribute：`0x8800_0000 ~ 0x8BFF_FFFF`
- 内置 ECC（Hamming）：
  - ECC 粒度可配置（`FMC_PCR.ECCPS`），结果读 `FMC_ECCR`。
  - 流程上需配合 `ECCEN` 开关与每页读写后的 ECC 读取。

## 2.4 SDRAM
- 两个 bank，分别由 `FMC_SDCR1/2`、`FMC_SDTR1/2` 控制。
- 初始化由软件发命令序列（`FMC_SDCMR`）：
  - 供时钟 -> 延时 -> Precharge All -> Auto Refresh -> Load Mode Register -> 刷新计数（`FMC_SDRTR`）。
- 关键提醒：
  - SDRAM 初始化参数最终以外部 SDRAM datasheet 为准（CAS、tRCD、tRP、刷新周期等）。

## 3. OctoSPI 关键结论（项目相关）

## 3.1 功能模式（`OCTOSPI_CR.FMODE[1:0]`）
- `00`：Indirect write
- `01`：Indirect read
- `10`：Automatic status-polling
- `11`：Memory-mapped

## 3.2 三类常用工作流
- Indirect 模式：
  - 通过 `DLR/CCR/TCR/IR/ABR/AR/DR` 组织命令帧与数据搬运。
  - FIFO 32B，`FTHRES` 定义阈值，配合 `FTF` 与 `DMAEN`。
- Status-polling 模式：
  - `PSMKR/PSMAR/PIR` 周期轮询状态。
  - `PMM` 控制 AND/OR 匹配，`APMS` 可匹配即停。
- Memory-mapped 模式：
  - 支持 XIP 读取预取；`TCEN + LPTR` 可在空闲时释放 NCS 降功耗。
  - 超 `DEVSIZE` 范围访问会触发 AXI 错误。

## 3.3 设备配置重点（`DCR1/2/3/4`）
- `MTYP[2:0]`：选择存储器协议/类型（含 HyperBus 模式）。
- `DEVSIZE[4:0]`：设备容量定义（公式：`bytes = 2^(DEVSIZE+1)`）。
- `CSHT[5:0]`：命令间 NCS 最小高电平周期。
- `DLYBYP`：是否绕过延迟块（DLYB）。
- `MAXTRAN`（`DCR3`）：通信调度限制（对复用/共享场景关键）。
- `DMM`：双存储器（dual-memory）配置开关。

## 3.4 协议与采样注意事项
- 支持 Regular-command 与 HyperBus 两协议。
- DQS 相关：
  - 开启 DQS 时建议满足手册给出的 dummy cycle 建议值。
  - 高速时通常需要 DLYB 做采样相位校准。
- 配置写入条件：
  - 多数 OctoSPI 配置寄存器要求 `BUSY=0` 时改写。
- 错误/状态位：
  - `SR`: `BUSY / TCF / TEF / FTF / SMF / TOF`
  - `FCR`: 对应清除位 `CTCF / CTEF / CSMF / CTOF`

## 3.5 中断
- 主要中断源与使能：
  - Timeout: `TOF / TOIE`
  - Status match: `SMF / SMIE`
  - FIFO threshold: `FTF / FTIE`
  - Transfer complete: `TCF / TCIE`
  - Transfer error: `TEF / TEIE`

## 4. OCTOSPIM 与 DLYB（和 OctoSPI 的关系）

## 4.1 OCTOSPIM（第 26 章）
- 作用：在 AF 复用前提供 OctoSPI IO 矩阵映射和多控制器复用能力。
- 关键点：
  - `OCTOSPIM_CR.MUXEN`：两路 OctoSPI 总线复用开关。
  - `REQ2ACK_TIME`：总线切换延时。
  - `OCTOSPIM_PnCR`：每个 Port 的 `CLKSRC/DQSSRC/NCSSRC` 选择与使能。
- 限制：
  - 改映射前需确保相关 OctoSPI 处于禁用状态。

## 4.2 DLYB（第 27 章）
- 作用：给采样时钟（尤其 DQS）增加可编程相移，提升高速采样窗口。
- 关键寄存器：
  - `DLYB_CR`: `DEN/SEN`
  - `DLYB_CFGR`: `UNIT/SEL/LNG/LNGF`
- 典型流程：
  - 先做长度采样校准（覆盖 1 个输入时钟周期），再选相位点（SEL）。

## 5. 面向当前项目的落地检查清单

- FMC（若要上外扩 SRAM/PSRAM/SDRAM/NAND）：
  - 确认地址窗口与 MPU 属性（尤其 NAND 建议 Device 属性）。
  - 明确外设时钟 `fmc_ker_ck` 与时序寄存器换算。
  - SDRAM 按芯片手册走完整初始化命令序列。
- OctoSPI（若要上 NOR/HyperRAM）：
  - 先确定协议（Regular-command / HyperBus）和数据线宽度。
  - 正确设置 `MTYP/DEVSIZE/CSHT/FMDOE/FTHRES`，并按 `BUSY=0` 规则配置。
  - 高速场景评估 DQS + DLYB 校准。
  - Memory-mapped 模式下配置 `TCEN/LPTR` 以权衡功耗与响应。

## 6. 备注

- 本文来自本地拆页文本，章节号和字段名已按 RM 原结构保留。
- 若后续要做驱动初始化代码，建议直接以第 24/25/26/27 章寄存器位定义逐条对照实现。
