# STM32H723 FMC 驱动 LCD 8080 16-bit 并口资料整理

## 1. 目标与范围

本整理面向 **STM32H723 + FMC + Intel 8080 16-bit LCD（带 GRAM）** 场景，优先收集一手资料（ST 官方手册/应用笔记、Zephyr 官方文档、TouchGFX 官方文档），并提炼为可执行的 bring-up 清单。

---

## 2. 必读一手资料（按优先级）

### A. 核心手册（必须）

1. RM0468（参考手册，FMC 章节）
- https://www.st.com/resource/en/reference_manual/rm0468-stm32h723733-stm32h725735-and-stm32h730-value-line-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- 重点：Chapter 24（FMC），尤其 `24.5/24.6/24.7.6/24.9.5/24.9.6`。

2. STM32H723 数据手册（电气时序，FMC characteristics）
- https://www.st.com/resource/en/datasheet/stm32h723zg.pdf
- 重点：`6.3.18 FMC characteristics`，异步 NOR/PSRAM 时序表和波形（用于 8080 时序约束）。

### B. 配置方法与经验（高价值）

3. AN2790（FSMC/FMC 驱动 8080 LCD 的经典方法）
- https://www.st.com/resource/en/application_note/an2790-tft-lcd-interfacing-with-the-highdensity-stm32f10xxx-fsmc-stmicroelectronics.pdf
- 价值：给出 8080 接线关系、RS 地址线映射思路、时序参数换算方法（`ADDSET/DATAST/ADDHOLD/ACCMOD`）。
- 说明：器件代际较老（F1/FSMC），但方法论仍可迁移到 H7/FMC。

4. AN4839（H7 Cache 与 DMA 一致性）
- https://www.st.com/resource/en/application_note/an4839-level-1-cache-on-stm32f7-series-and-stm32h7-series-stmicroelectronics.pdf
- 重点：DMA 缓冲区 cache 一致性、clean/invalidate、non-cacheable 区域建议。

5. AN4838（MPU 属性管理）
- https://www.st.com/resource/en/application_note/an4838-managing-memory-protection-unit-in-stm32-mcus-stmicroelectronics.pdf
- 重点：Normal/Device memory、XN、Shareable、TEX/C/B/S 组合。

### C. 框架与工程化（Zephyr / TouchGFX）

6. TouchGFX：FMC Display Interface 场景
- https://support.touchgfx.com/docs/development/touchgfx-hal-development/scenarios/scenarios-fmc
- 重点：CubeMX 把 FMC Bank 设为 LCD Interface；16-bit 接口可生成读写寄存器/数据弱符号 API；TE 同步策略；DMA 传输策略。

7. TouchGFX：Display 选型（接口带宽与推荐分辨率）
- https://support.touchgfx.com/docs/development/hardware-selection/hardware-components/hardware-selection-display
- 重点：8080/6800(FMC) 的性能定位与分辨率建议。

8. Zephyr：STM32H7 FMC Devicetree 绑定
- https://docs.zephyrproject.org/latest/build/dts/api/bindings/memory-controllers/st%2Cstm32h7-fmc.html
- 重点：`st,stm32h7-fmc`、`st,mem-swap`、多内存控制器共享信号/时钟关系。

9. Zephyr：Display / MIPI-DBI 接口
- https://docs.zephyrproject.org/latest/hardware/peripherals/display/index.html
- https://docs.zephyrproject.org/latest/doxygen/html/group__mipi__dbi__interface.html
- https://docs.zephyrproject.org/latest/build/dts/api/bindings/mipi-dbi/zephyr%2Cmipi-dbi-bitbang.html
- 重点：Zephyr display 抽象、DBI Type B(8080) 语义、bitbang A/B 参考实现。

---

## 3. 关键结论（已从资料提炼）

1. **FMC 可直接覆盖 8080 LCD 控制器场景**
RM/DS 均明确 FMC 面向外部静态存储类时序，8080 LCD 在实现上可按“异步 NOR/PSRAM 风格总线”建模（命令/数据通过地址线区分）。

2. **时序参数必须用“LCD datasheet + FMC 时序表”双向约束**
不要套固定值。应从 LCD 的 `tAS/tAH/tWRL/tWRH/tDS/tDH/tCYC` 回算 `ADDSET/DATAST/ADDHOLD/ACCMOD`，并校验不违反 H723 数据手册的 FMC 约束。

3. **H7 上 cache/MPU 是稳定性关键项，不是可选项**
若用 DMA 刷图或部分刷新，必须提前设计缓存一致性策略：
- DMA buffer 放 non-cacheable 区；或
- 严格 clean/invalidate；或
- MPU 改为合适共享/写策略。

4. **Zephyr 原生需要补“FMC->显示驱动”适配层**
Zephyr 有 FMC 控制器绑定和 display/DBI 抽象，但实际 STM32H7 FMC 8080 LCD 通常需要你实现/扩展具体显示驱动（或先以 TouchGFX/Cube 方案验证硬件时序）。

---

## 4. 建议的 bring-up 顺序（实操）

1. 仅写命令：确认 `CS/RS/WR` 波形正确。
2. 写单像素：确认 RGB565 颜色字节序。
3. GRAM 连续写：验证地址窗口/自动递增。
4. 全屏纯色循环：观察是否有花屏、抖动、写丢失。
5. 接入 DMA：先小块，再整帧，最后加 cache 维护。
6. 加 TE 同步（若 LCD 提供）：避免撕裂。
7. 在 Zephyr 集成：先最小 `display_write()` 通路，再做局部刷新优化。

---

## 5. 本仓库内可联动文档

- [stm32h723_octospi_fmc_summary.md](C:\env\zephyrproject\OmniGenH7\docs\stm32h723_octospi_fmc_summary.md)
- [digital_signal_generator_architecture_report.md](C:\env\zephyrproject\OmniGenH7\docs\digital_signal_generator_architecture_report.md)
- [embedded_framework_design_guide.md](C:\env\zephyrproject\OmniGenH7\docs\embedded_framework_design_guide.md)
