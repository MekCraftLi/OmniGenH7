# DAC 模拟滤波器方案交接

日期：2026-05-17

## 背景与目标

当前项目使用 `STM32H723ZG` 内部 DAC 作为模拟波形输出源，主输出引脚为 `PA4 / DAC1_OUT1`。目标是在 `1 MS/s` DAC 更新率下，通过外部模拟滤波器和高速运放改善 DAC 阶梯输出，支持正弦、AM、FM、三角波、方波、锯齿波、Sinc、扫频和任意波输出。

本方案定位为通用波形发生器的模拟重建滤波方案。核心目标是：

- 对正弦、AM、FM、Sinc、扫频提供较好的重建效果。
- 对三角波、锯齿波提供可接受的限带输出。
- 对方波保留旁路/宽带输出，不把 DAC + 低通作为高质量方波方案。
- 通过多档滤波器兼顾低频噪声抑制和高频波形带宽。

## 当前结论

推荐采用可切换模拟滤波器组：

```text
DAC_OUT(PA4)
  -> 输入保护/轻 RC
  -> 模拟开关输入选择
      -> Bypass/Wide
      -> Low  25 kHz  4阶有源低通
      -> Mid  125 kHz 4阶有源低通
      -> High 225 kHz 4阶有源低通
  -> 模拟开关输出选择
  -> 输出缓冲/增益级
  -> 输出保护
  -> OUT
```

默认建议每个低通档位使用 `4阶有源 Butterworth 低通`。硬件实现优先考虑 `MFB` 结构，即两个二阶 MFB 低通级联：

```text
4阶低通 = 2阶 MFB(Q=0.5412) + 2阶 MFB(Q=1.3065)
```

低 Q 级建议放在前面，高 Q 级放在后面：

```text
DAC -> MFB Q=0.5412 -> MFB Q=1.3065 -> buffer
```

## 滤波器档位

| 档位 | 建议截止频率 | 推荐实现 | 主要用途 |
|---|---:|---|---|
| Bypass/Wide | `>500 kHz` 或旁路 | 宽带缓冲，不做重建低通 | 方波、调试 DAC 阶梯输出、低保真任意波。 |
| Low | `25 kHz` | 4阶有源低通，优先 MFB | `1~10 kHz` 正弦、AM/FM、三角波。 |
| Mid | `125 kHz` | 4阶有源低通，优先 MFB | `10~50 kHz` 正弦、AM/FM、扫频、部分任意波。 |
| High | `225 kHz` | 4阶有源低通，优先 MFB | `50~100 kHz` 正弦、窄带 AM/FM、扫频高频端。 |

如果硬件资源不足，只能保留一个滤波器，建议优先做 `150~200 kHz` 的 `3~4阶有源低通`，它可以覆盖正弦、AM、FM、扫频主场景，但低频噪声抑制和高频镜像抑制都不是最优。

## 波形与滤波器匹配

| 波形 | 推荐档位 | 说明 |
|---|---|---|
| Sine 正弦 | `Mid` 或 `High` | `50 kHz` 内建议 `Mid`，`50~100 kHz` 建议 `High`。 |
| AM 调幅 | `Mid` 或 `High` | 截止频率必须覆盖最高边带，避免包络压缩。 |
| FM 调频 | `Mid` 或 `High` | 最高瞬时频率为 `f_carrier + f_deviation`，频偏大时应降低载波。 |
| Triangle 三角波 | `Low` 或 `Mid` | 需要保留高次谐波，频率越高越容易变圆。 |
| Square 方波 | `Bypass/Wide` | 高质量方波建议使用定时器/GPIO/比较器路径，不建议依赖 DAC + 低通。 |
| Sawtooth 锯齿波 | `Low` 或 `Mid` | 高频时谐波不足，低通会让斜率和尖角变钝。 |
| Sinc | `High` | 按目标带宽选择，当前仿真使用 `100 kHz` 带宽对应 `High`。 |
| Sweep 扫频 | `Mid` 或 `High` | 终点 `50 kHz` 用 `Mid`，终点 `100 kHz` 用 `High`。 |
| Arbitrary 任意波 | 按最高有效频谱选档 | 快速边沿、尖峰、窄脉冲应按方波/锯齿波保守处理。 |

## 频率建议

在 `1 MS/s` DAC 更新率下：

```text
100 kHz = 10 points/cycle
50 kHz  = 20 points/cycle
20 kHz  = 50 points/cycle
10 kHz  = 100 points/cycle
```

当前建议规格：

| 波形 | 高质量建议上限 | 扩展可用上限 |
|---|---:|---:|
| Sine | `50 kHz` | `100 kHz` |
| AM | `30 kHz` carrier | `50 kHz` carrier |
| FM | `20~30 kHz` carrier | `50 kHz` carrier，频偏要小 |
| Triangle | `10~20 kHz` | `50 kHz` |
| Square | `10 kHz` | `20~50 kHz`，边沿明显圆滑 |
| Sawtooth | `10 kHz` | `20 kHz` 左右 |
| Sinc | `100 kHz` 带宽/主瓣参数 | 取决于幅度和滤波器 |
| Sweep | 终止 `50 kHz` | 终止 `100 kHz`，高频端质量下降 |
| Arbitrary | 最高有效频谱 `50 kHz` | 最高有效频谱 `100 kHz`，质量下降 |

## 为什么不使用带通滤波器

当前输出是通用基带波形发生器，任务是 DAC 重建滤波，不是固定载波通信接收/发射链路。通用输出不建议使用带通滤波器。

原因：

- 带通会破坏低频波形、DC 偏置、扫频起始段和任意波低频成分。
- AM/FM 可以是窄带，但本项目还要支持正弦、三角、方波、锯齿、Sinc、扫频和任意波。
- DAC 的主要问题是阶梯高频分量和 `1 MHz +/- f_signal` 镜像，低通重建滤波更直接。

带通只适合独立的固定载波窄带 AM/FM 测试源，不适合作为当前通用输出主路径。

## MFB 实现说明

MFB 是 `Multiple Feedback`，即多重反馈有源滤波器。它使用运放、电阻、电容构成二阶滤波单元，常用于较高频率的有源低通。

相对 Sallen-Key，MFB 的特点：

- 更适合 `125 kHz`、`225 kHz` 这类较高截止频率。
- 高 Q 二阶级更容易控制。
- 四阶只需要两个运放。
- 输入阻抗较低，且单级反相。
- 单电源系统中必须围绕模拟中点电压工作，例如 `Vref = 1.65 V`。

单级 MFB 二阶低通应围绕 `Vref` 偏置，而不是直接参考数字地。两级 MFB 级联后反相两次，最终相位方向恢复。

设计时不要手工猜 R/C。建议使用 TI Filter Design Tool、ADI Filter Wizard 或 SPICE 工具，按下面参数生成每个档位：

```text
Filter type: 4th-order Butterworth low-pass
Stage 1 Q: 0.5412
Stage 2 Q: 1.3065
Gain: 1 V/V unless output scaling is required
Vref: 1.65 V
fc: 25 kHz / 125 kHz / 225 kHz
```

推荐器件指标：

```text
GBW >= 50 MHz
Slew Rate >= 20 V/us
单电源 5 V 优先
轨到轨输入/输出优先
低噪声
能稳定驱动后级负载和少量电容
```

## GPIO 与模拟开关控制

当前引脚规划中已有滤波器相关 GPIO：

| 信号 | 引脚 | 方向 | 用途 |
|---|---|---|---|
| `FILTER_SEL0` | `PB0` | GPIO 输出 | 模拟开关选择位 0 |
| `FILTER_SEL1` | `PB1` | GPIO 输出 | 模拟开关选择位 1 |
| `FILTER_EN` | `PB2` | GPIO 输出 | 模拟开关/滤波器使能 |
| `OUTPUT_MUTE` | `PB10` | GPIO 输出 | 切换时静音或断开输出 |

建议编码：

| `SEL1` | `SEL0` | 档位 |
|---:|---:|---|
| 0 | 0 | Bypass/Wide |
| 0 | 1 | Low 25 kHz |
| 1 | 0 | Mid 125 kHz |
| 1 | 1 | High 225 kHz |

建议切换流程：

```text
1. OUTPUT_MUTE = active
2. FILTER_EN = inactive
3. 设置 FILTER_SEL0 / FILTER_SEL1
4. 等待模拟开关地址稳定
5. FILTER_EN = active
6. 等待滤波器输出稳定
7. OUTPUT_MUTE = inactive
```

建议稳定时间先按 `2~10 ms` 处理，后续根据实测缩短。

## 当前固件状态

当前仓库只记录了滤波器 GPIO 的引脚规划，还没有固件适配：

- `docs/pin_assignment_summary.md` 已规划 `PB0/PB1/PB2/PB10`。
- `boards/st/omnigen_h7/omnigen_h7.dts` 尚未定义滤波器 GPIO 节点或 alias。
- 没有 `AnalogFilterSwitch` / `FilterBank` 平台模块。
- 没有滤波器切换 shell 命令。
- 没有将滤波器档位与波形/频率自动匹配。
- 当前真实 DAC 输出链路也尚未完整接入应用层，`SignalEngine` 仍注入 `MockWaveSink`。

## MATLAB 仿真依据

相关脚本：

```text
simulation/matlab/run_1mhz_filter_suite.m
simulation/matlab/simulate_recommended_filters.m
```

输出目录：

```text
simulation/matlab/output_1mhz_filter_suite/
simulation/matlab/output_filter_recommendations/
```

重点结果文件：

```text
simulation/matlab/output_filter_recommendations/filter_bank_response.png
simulation/matlab/output_filter_recommendations/recommended_waveform_filter_results.png
simulation/matlab/output_filter_recommendations/extension_limit_filter_results.png
simulation/matlab/output_filter_recommendations/active_vs_passive_comparison.png
simulation/matlab/output_filter_recommendations/recommended_filter_metrics.csv
```

复现实验命令：

```powershell
& 'C:\Program Files\MATLAB\R2024b\bin\matlab.exe' -batch "cd('C:/env/zephyrproject/OmniGenH7/simulation/matlab'); results = simulate_recommended_filters();"
```

注意：MATLAB 中滤波展示使用 `filtfilt` 做离线零相位仿真，便于观察幅频效果。真实硬件有源滤波器会产生相位延迟，已在 `filter_bank_response.png` 中展示群延迟估计。

## 后续任务建议

1. 在 DTS 中添加滤波器 GPIO 节点或 aliases：

```text
filter_sel0 = PB0
filter_sel1 = PB1
filter_en   = PB2
output_mute = PB10
```

2. 新增平台模块：

```text
inc/platform/analog_filter_switch.hpp
src/platform/analog_filter_switch.cpp
```

建议接口：

```cpp
enum class FilterMode {
    Bypass,
    Low25k,
    Mid125k,
    High225k,
};

class AnalogFilterSwitch {
public:
    Result<void> init();
    Result<void> set_mode(FilterMode mode);
    FilterMode mode() const;
};
```

3. 在 shell 中增加调试命令：

```text
filter status
filter set bypass
filter set low
filter set mid
filter set high
filter mute on
filter mute off
```

4. 将滤波器选择与波形/频率策略绑定：

```text
Sine <= 50 kHz       -> Mid
Sine > 50 kHz        -> High
AM/FM <= 30 kHz      -> Mid
AM/FM 30~50 kHz      -> High
Triangle <= 10 kHz   -> Low
Triangle 10~50 kHz   -> Mid/High depending on quality target
Square               -> Bypass/Wide
Sweep stop <= 50 kHz -> Mid
Sweep stop > 50 kHz  -> High
```

5. 硬件 bring-up 时按档位测量：

```text
1 kHz sine
10 kHz sine
50 kHz sine
100 kHz sine
30 kHz AM
30 kHz FM
20 kHz triangle
50 kHz triangle
```

记录每档的幅度衰减、相位延迟、THD、噪声和过冲，并建立幅度校准表。
