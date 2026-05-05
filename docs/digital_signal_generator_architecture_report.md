# OmniGenH7 Digital Signal Generator Architecture Report

本文档面向当前 OmniGenH7 数字信号发生器项目，给出建议的文件结构、线程分布、中断路由、核心变量和类设计、状态机、数据流以及 Zephyr 落地步骤。当前仓库仍处于 Zephyr 最小应用阶段，`src/main.c` 主要验证 USART1 输出；本文档描述的是后续工程化重构目标。

## 1. 项目目标

当前产品目标：

- MCU：STM32H723ZG。
- RTOS：Zephyr。
- 存储：QSPI/OSPI/SPI NOR Flash，目标器件 W25Q64。
- 显示：FMC 驱动 8080 并口 480x320 屏幕。
- 输入：若干按键和一个编码器。
- 输出：通过 DAC 输出数字信号，建议使用 TIM + DMA + DAC 形成稳定采样时基。
- 通信：预留 UART/USB CDC/其他上位机通信接口。

架构目标：

- `domain` 不依赖 Zephyr、HAL、驱动和 RTOS。
- `services` 只依赖 `ports` 和 `domain`。
- `drivers/platform` 实现硬件和 Zephyr 适配。
- `app` 作为 composition root，集中装配对象和启动线程。
- ISR 只做事件通知，不做业务逻辑。
- 实时输出链路不做 heap 分配、不做文件 IO、不做复杂日志。
- UI、Shell、上位机查询通过状态快照观察系统，不直接读内部变量。

## 2. 推荐目录结构

建议把当前工程逐步迁移为以下结构：

```text
OmniGenH7/
  CMakeLists.txt
  Kconfig
  prj.conf

  boards/st/omnigen_h7/
    omnigen_h7.dts
    omnigen_h7_defconfig
    board.cmake
    board.yml
    Kconfig.defconfig
    Kconfig.omnigen_h7
    support/openocd.cfg

  dts/bindings/
    display/mekcraft,fmc-8080-bus.yaml
    output/mekcraft,dac-wave-sink.yaml

  include/omnigen/
    base/
      result.hpp
      error.hpp
      units.hpp
      fixed_buffer.hpp
      ring_buffer.hpp

    domain/
      waveform.hpp
      signal_profile.hpp
      signal_limits.hpp
      calibration.hpp
      preset.hpp
      ui_model.hpp

    ports/
      storage_port.hpp
      waveform_file_port.hpp
      wave_sink_port.hpp
      display_port.hpp
      input_port.hpp
      time_port.hpp
      settings_port.hpp
      diagnostics_port.hpp

  drivers/
    display/
      fmc_8080_bus.c
      panel_480x320.c

    output/
      dac_wave_sink.c

    input/
      board_buttons.c
      board_encoder.c

    storage/
      w25q64_support.c

  src/
    app/
      main.cpp
      composition_root.cpp
      app_config.cpp

    platform/
      zephyr/
        zephyr_time.cpp
        zephyr_storage.cpp
        zephyr_settings.cpp
        zephyr_display.cpp
        zephyr_input.cpp
        zephyr_wave_sink.cpp
        zephyr_threads.cpp
        zephyr_message_queue.cpp

    domain/
      waveform_synthesis.cpp
      waveform_codec.cpp
      signal_validation.cpp
      calibration.cpp

    services/
      signal_engine.cpp
      output_scheduler.cpp
      waveform_repository.cpp
      input_router.cpp
      ui_presenter.cpp
      host_protocol_service.cpp
      settings_service.cpp
      system_supervisor.cpp

    ui/
      screens/
        main_screen.cpp
        waveform_screen.cpp
        storage_screen.cpp
        system_screen.cpp
      widgets/

    diagnostics/
      shell_commands.cpp
      metrics.cpp
      state_snapshot.cpp
      fault_reporter.cpp

  tests/
    unit/
    native_sim/
    hardware_smoke/

  docs/
    embedded_framework_design_guide.md
    digital_signal_generator_architecture_report.md
```

### 2.1 目录职责

| 目录 | 职责 | 禁止事项 |
| --- | --- | --- |
| `include/omnigen/base` | 通用基础类型、错误、单位、固定缓冲 | 不依赖 Zephyr |
| `include/omnigen/domain` | 波形、校准、参数、限制、业务概念 | 不包含设备句柄 |
| `include/omnigen/ports` | 能力接口 | 不包含具体驱动 |
| `drivers` | Zephyr C driver 或底层硬件 glue | 不写业务状态机 |
| `src/platform/zephyr` | Zephyr API 到 ports 的适配 | 不写 UI/业务策略 |
| `src/services` | 业务编排、状态机、线程主体 | 不直接访问寄存器 |
| `src/app` | 对象创建、依赖注入、启动 | 不实现复杂业务 |
| `src/ui` | 页面、控件、UI 事件转换 | 不直接操作 DAC/Flash |
| `src/diagnostics` | Shell、日志、指标、状态快照 | 不改变业务状态，除显式调试命令 |

## 3. 依赖关系

目标依赖图：

```text
app
  -> services
  -> platform/zephyr
  -> drivers

services
  -> ports
  -> domain
  -> base

platform/zephyr
  -> ports
  -> Zephyr APIs

drivers
  -> Zephyr driver model
  -> STM32 LL/HAL where necessary

domain
  -> base only
```

禁止依赖：

- `domain` 依赖 Zephyr、HAL、drivers、services。
- `services` 直接 include `zephyr/drivers/*`。
- `ui` 直接调用 DAC、Flash、FMC、DMA。
- `drivers` 调用 UI 或业务 service。
- ISR 调用文件系统、UI 或复杂业务逻辑。

## 4. 硬件模块划分

| 硬件/能力 | 类型 | 建议归属 | 上层抽象 |
| --- | --- | --- | --- |
| STM32 DAC | MCU 外设 | `drivers/output/dac_wave_sink.c` | `WaveSinkPort` |
| TIM | MCU 外设 | `drivers/output/dac_wave_sink.c` 内部使用 | 不直接暴露 |
| DMA | MCU 外设 | `drivers/output/dac_wave_sink.c` 内部使用 | 不直接暴露 |
| QSPI/OSPI/SPI NOR | MCU 外设 + 外部 Flash | Zephyr flash driver / `drivers/storage` | `StoragePort` |
| W25Q64 | 外部设备 | Zephyr SPI NOR 或 support glue | `StoragePort` |
| FMC | MCU 外设 | `drivers/display/fmc_8080_bus.c` | `DisplayPort` transport |
| 8080 屏幕控制器 | 外部设备 | `drivers/display/panel_480x320.c` | `DisplayPort` |
| GPIO 按键 | 外部输入设备 | Zephyr GPIO/Input 或 `drivers/input` | `InputPort` |
| 编码器 | 外部输入设备 | Zephyr input/QDEC/GPIO | `InputPort` |
| UART 上位机 | 通信外设 | Zephyr UART async adapter | `HostProtocolService` |

## 5. 核心数据流

系统应拆成三条数据通道：控制流、实时数据流、状态流。

### 5.1 控制流

控制流处理用户命令、按键、编码器、上位机命令：

```text
buttons / encoder / host uart
  -> InputPort / HostProtocolService
  -> InputRouter
  -> SignalCommand
  -> SignalEngine
  -> OutputScheduler / WaveformRepository / UiPresenter
```

控制流中的数据结构：

- `InputEvent`
- `HostCommand`
- `SignalCommand`
- `UiAction`
- `SystemEvent`

### 5.2 实时输出数据流

实时输出数据流要尽量短、确定、无阻塞：

```text
WaveformRepository
  -> WaveformDecoder / WaveformSynthesis
  -> OutputScheduler
  -> DmaBlockPool
  -> WaveSinkPort
  -> TIM + DMA + DAC
```

实时路径规则：

- 不访问文件系统。
- 不动态分配内存。
- 不等待 UI。
- 不使用阻塞 mutex。
- 只允许轻量计数器或采样型诊断。

### 5.3 状态流

状态流用于 UI、Shell、日志和上位机查询：

```text
SignalEngine / OutputScheduler / Storage / InputRouter
  -> immutable snapshots
  -> UiPresenter / diagnostics shell / host query
```

状态快照应是结构体拷贝，不应返回内部可变引用。

## 6. 线程分布

建议初期线程不要过多。优先建立 5 个核心线程，后续再拆。

| 线程 | 建议优先级 | 栈建议 | 输入 | 输出 | 职责 |
| --- | --- | --- | --- | --- | --- |
| `output_thread` | 高 | 2048-4096 | 输出命令、DMA 回调事件 | DAC DMA buffer | 输出调度、buffer refill、启动停止 |
| `control_thread` | 中高 | 4096 | 按键/编码器/上位机命令 | service command | 系统主控制状态机 |
| `ui_thread` | 中 | 4096-8192 | 状态快照、UI 事件 | display flush | 页面刷新和交互反馈 |
| `storage_thread` | 中低 | 4096 | 文件请求 | 波形块、文件结果 | 文件枚举、读取、保存、删除 |
| `diagnostics_thread` | 低 | 2048-4096 | shell/query/log event | 文本输出、指标 | 调试命令、状态导出 |

### 6.1 线程职责细化

#### `output_thread`

归属：`src/services/output_scheduler.cpp`

职责：

- 管理输出状态机。
- 维护 DMA block pool 和输出 ring buffer。
- 响应 `StartOutput`、`StopOutput`、`PauseOutput`、`UpdateProfile`。
- 响应 DMA half/full complete 事件。
- 处理 underrun、overrun、采样率非法、buffer 不足。

不应做：

- 从 Flash 读文件。
- 解析上位机协议。
- 绘制 UI。
- 长时间持锁。

#### `control_thread`

归属：`src/services/signal_engine.cpp` 或 `src/services/system_supervisor.cpp`

职责：

- 作为系统主状态机入口。
- 接收 `InputRouter` 和 `HostProtocolService` 的命令。
- 校验命令是否合法。
- 协调 `WaveformRepository`、`OutputScheduler`、`SettingsService`。
- 生成 UI 需要的状态变化。

#### `ui_thread`

归属：`src/ui/` 与 `src/services/ui_presenter.cpp`

职责：

- 定期或按事件获取状态快照。
- 渲染当前页面。
- 将输入事件转换为 `UiAction`，交给 `InputRouter`。
- 管理屏幕局部刷新。

#### `storage_thread`

归属：`src/services/waveform_repository.cpp`

职责：

- 管理 LittleFS 或其他文件系统访问。
- 维护波形文件索引。
- 执行保存、删除、加载、枚举。
- 将大文件读取拆成 block。

#### `diagnostics_thread`

归属：`src/diagnostics/`

职责：

- 提供 shell 命令。
- 导出状态快照。
- 输出关键指标。
- 支持故障报告和简单自检。

## 7. 中断路由

ISR 应遵循：

```text
硬件中断
  -> 极薄 ISR/callback
  -> ISR-safe event/queue/atomic flag
  -> 对应线程处理
```

### 7.1 DAC/TIM/DMA 中断

```text
DMA half complete IRQ
  -> dac_wave_sink_isr()
  -> k_msgq_put(output_isr_queue, DmaHalfComplete)
  -> output_thread refill first half

DMA complete IRQ
  -> dac_wave_sink_isr()
  -> k_msgq_put(output_isr_queue, DmaComplete)
  -> output_thread refill second half

DMA error IRQ
  -> dac_wave_sink_isr()
  -> atomic error flag + notify output_thread
  -> output_thread transitions to Fault
```

ISR 只允许：

- 清中断标志。
- 记录计数器。
- 投递 `OutputIsrEvent`。
- 必要时停止硬件输出以保护设备。

### 7.2 按键和编码器中断

```text
GPIO EXTI / input callback
  -> input adapter
  -> k_msgq_put(input_event_queue, RawInputEvent)
  -> input_router debounce / map to command
```

编码器如使用 QDEC 或 Zephyr input subsystem，应尽量让平台层统一输出 `InputEvent`。

### 7.3 UART 上位机中断

```text
UART async callback
  -> zephyr uart adapter
  -> rx ring buffer
  -> notify host_protocol_service
  -> parser builds HostCommand
```

UART callback 不解析完整业务命令，只做 buffer 管理和线程通知。

### 7.4 Flash 相关中断

Flash 文件系统访问不应出现在 ISR。若底层驱动有异步完成回调，也只通知 `storage_thread`。

### 7.5 显示刷新中断

如果屏幕使用 DMA/FMC 异步刷新：

```text
display transfer complete IRQ
  -> display adapter
  -> notify ui_thread
```

UI 绘制和页面状态切换不进入 ISR。

## 8. 核心类和接口

下面列出建议的核心类型。初期可以不用全部实现，但命名和职责应尽量稳定。

### 8.1 `base`

```cpp
enum class ErrorCode {
    ok,
    invalid_argument,
    invalid_state,
    timeout,
    io_error,
    busy,
    no_memory,
    unsupported,
    hardware_fault,
    data_corrupted,
};

template <typename T>
class Result;

struct FrequencyHz {
    uint32_t value;
};

struct VoltageMv {
    int32_t value;
};

struct SampleRateHz {
    uint32_t value;
};
```

基础类型要求：

- 不依赖 Zephyr。
- 可在 native unit test 中使用。
- 低成本拷贝。
- 单位明确，避免裸 `uint32_t`。

### 8.2 `domain`

```cpp
enum class WaveformKind {
    sine,
    square,
    triangle,
    sawtooth,
    arbitrary,
};

struct SignalProfile {
    WaveformKind kind;
    FrequencyHz frequency;
    SampleRateHz sample_rate;
    VoltageMv amplitude;
    VoltageMv offset;
    uint16_t duty_permille;
    bool output_enabled;
};

struct SignalLimits {
    FrequencyHz min_frequency;
    FrequencyHz max_frequency;
    SampleRateHz min_sample_rate;
    SampleRateHz max_sample_rate;
    VoltageMv min_voltage;
    VoltageMv max_voltage;
};

struct CalibrationData {
    int32_t dac_offset_lsb;
    int32_t dac_gain_ppm;
    uint32_t version;
    uint32_t crc32;
};

struct WaveformMeta {
    uint32_t id;
    char name[32];
    WaveformKind kind;
    SampleRateHz sample_rate;
    uint32_t sample_count;
    uint32_t crc32;
};
```

`domain` 中还应包含：

- `validate_signal_profile(profile, limits)`
- `synthesize_waveform(profile, span)`
- `encode_waveform_file(...)`
- `decode_waveform_file(...)`
- `apply_calibration(raw_sample, calibration)`

### 8.3 `ports`

```cpp
class WaveSinkPort {
public:
    virtual ~WaveSinkPort() = default;

    [[nodiscard]] virtual Result<void> configure(const SignalProfile& profile) = 0;
    [[nodiscard]] virtual Result<void> start() = 0;
    [[nodiscard]] virtual Result<void> stop() = 0;
    [[nodiscard]] virtual Result<void> submit_block(std::span<const uint16_t> samples) = 0;
    [[nodiscard]] virtual WaveSinkStatus status() const = 0;
};
```

```cpp
class StoragePort {
public:
    virtual ~StoragePort() = default;

    [[nodiscard]] virtual Result<void> mount() = 0;
    [[nodiscard]] virtual Result<FileHandle> open(FilePath path, OpenMode mode) = 0;
    [[nodiscard]] virtual Result<size_t> read(FileHandle& file, std::span<std::byte> out) = 0;
    [[nodiscard]] virtual Result<size_t> write(FileHandle& file, std::span<const std::byte> in) = 0;
    [[nodiscard]] virtual Result<void> close(FileHandle& file) = 0;
};
```

```cpp
class InputPort {
public:
    virtual ~InputPort() = default;

    [[nodiscard]] virtual Result<InputEvent> wait_event(Timeout timeout) = 0;
};
```

```cpp
class DisplayPort {
public:
    virtual ~DisplayPort() = default;

    [[nodiscard]] virtual Result<void> flush(Rect area, std::span<const Rgb565> pixels) = 0;
    [[nodiscard]] virtual DisplayStatus status() const = 0;
};
```

接口设计要求：

- 接口表达能力，不表达硬件型号。
- 接口数量少，语义明确。
- 不要建立万能 `IDevice`。
- 设备共同能力可用小接口表达，如 `Resettable`、`HealthCheckable`。

### 8.4 `services`

#### `SignalEngine`

职责：

- 系统主业务状态机。
- 接收控制命令。
- 校验参数。
- 协调输出、存储、设置、UI 状态。

核心成员：

```cpp
class SignalEngine {
public:
    Result<void> handle_command(const SignalCommand& command);
    SignalEngineSnapshot snapshot() const;

private:
    OutputScheduler& output_;
    WaveformRepository& repository_;
    SettingsService& settings_;
    SignalProfile active_profile_;
    SignalEngineState state_;
    FaultCode last_fault_;
};
```

#### `OutputScheduler`

职责：

- 管理实时输出。
- 维护 DMA buffer。
- 与 `WaveSinkPort` 交互。
- 处理 underrun/fault。

核心成员：

```cpp
class OutputScheduler {
public:
    Result<void> configure(const SignalProfile& profile);
    Result<void> start();
    Result<void> stop();
    void on_dma_event(OutputIsrEvent event);
    OutputSnapshot snapshot() const;

private:
    WaveSinkPort& sink_;
    SignalProfile profile_;
    OutputState state_;
    FixedBlockPool<uint16_t, kSamplesPerBlock, kBlockCount> block_pool_;
    RingBuffer<BlockRef, kOutputQueueDepth> ready_blocks_;
    OutputMetrics metrics_;
};
```

#### `WaveformRepository`

职责：

- 文件系统访问。
- 波形文件索引。
- 读写波形数据。

核心成员：

```cpp
class WaveformRepository {
public:
    Result<void> mount();
    Result<WaveformList> list();
    Result<WaveformMeta> meta(WaveformId id);
    Result<size_t> read_block(WaveformId id, uint32_t offset, std::span<uint16_t> out);
    Result<void> save(WaveformMeta meta, std::span<const uint16_t> samples);

private:
    StoragePort& storage_;
    RepositoryState state_;
    WaveformIndex index_;
};
```

#### `InputRouter`

职责：

- 处理按键、编码器、上位机输入。
- 去抖。
- 将低级事件映射为业务命令。

核心成员：

```cpp
class InputRouter {
public:
    Result<void> handle_event(const InputEvent& event);
    Result<void> handle_host_command(const HostCommand& command);

private:
    CommandQueue& command_queue_;
    UiFocusState focus_;
    DebounceState debounce_;
};
```

#### `UiPresenter`

职责：

- 从 snapshots 生成 UI model。
- 驱动页面刷新。
- 不直接操作业务对象的内部状态。

核心成员：

```cpp
class UiPresenter {
public:
    void tick();
    void request_redraw();

private:
    DisplayPort& display_;
    SnapshotProvider& snapshots_;
    UiModel current_model_;
};
```

#### `SystemSupervisor`

职责：

- 模块启动顺序。
- 故障升级。
- 看门狗喂狗策略。
- 运行健康检查。

核心成员：

```cpp
class SystemSupervisor {
public:
    Result<void> boot();
    void tick();
    SystemSnapshot snapshot() const;

private:
    BootState boot_state_;
    FaultReporter& faults_;
    WatchdogPort* watchdog_;
};
```

## 9. 关键变量和资源表

### 9.1 业务状态变量

| 变量 | 类型 | 归属 | 说明 |
| --- | --- | --- | --- |
| `active_profile_` | `SignalProfile` | `SignalEngine` | 当前输出配置 |
| `pending_profile_` | `SignalProfile` | `SignalEngine` | 待应用配置 |
| `signal_state_` | `SignalEngineState` | `SignalEngine` | 主状态机 |
| `output_state_` | `OutputState` | `OutputScheduler` | 输出状态 |
| `last_fault_` | `FaultCode` | `SignalEngine/SystemSupervisor` | 最近故障 |
| `calibration_` | `CalibrationData` | `SettingsService` | DAC 校准 |
| `selected_waveform_` | `WaveformId` | `SignalEngine/UI` | 当前选择波形 |

### 9.2 实时输出资源

| 资源 | 建议归属 | 说明 |
| --- | --- | --- |
| `dma_buffer` | `drivers/output` 或 `OutputScheduler` | DAC DMA 双缓冲 |
| `block_pool` | `OutputScheduler` | 固定块池，避免 heap |
| `ready_blocks` | `OutputScheduler` | 待输出 block 队列 |
| `output_isr_queue` | platform/driver | DMA ISR 到输出线程 |
| `sample_cursor` | `OutputScheduler` | 当前波形读取位置 |
| `underrun_count` | `OutputMetrics` | 输出断流计数 |
| `dma_error_count` | `OutputMetrics` | DMA 错误计数 |

### 9.3 存储资源

| 资源 | 建议归属 | 说明 |
| --- | --- | --- |
| `fs_mount` | `ZephyrStorage` | LittleFS mount |
| `flash_area` | Zephyr flash map | Flash 分区 |
| `waveform_index` | `WaveformRepository` | 波形索引 |
| `file_handle` | RAII wrapper | 文件生命周期 |
| `storage_queue` | `storage_thread` | 文件请求队列 |

### 9.4 输入和 UI 资源

| 资源 | 建议归属 | 说明 |
| --- | --- | --- |
| `input_event_queue` | `ZephyrInput` / `InputRouter` | 按键编码器事件 |
| `debounce_state` | `InputRouter` | 去抖状态 |
| `encoder_position` | `InputRouter` | 编码器累计量 |
| `ui_model` | `UiPresenter` | 当前显示模型 |
| `framebuffer` | `UiPresenter` 或 display driver | 显示缓冲 |
| `display_flush_queue` | display adapter | 刷新完成通知 |

### 9.5 诊断资源

| 资源 | 建议归属 | 说明 |
| --- | --- | --- |
| `SystemSnapshot` | `diagnostics` | 全局状态快照聚合 |
| `OutputMetrics` | `OutputScheduler` | 输出性能指标 |
| `StorageMetrics` | `WaveformRepository` | 存储耗时和错误 |
| `FaultLog` | `FaultReporter` | 故障记录 |
| `shell_commands` | `diagnostics` | 调试命令 |

## 10. 状态机设计

建议将系统拆成多个小状态机，而不是一个巨大状态机。

### 10.1 系统启动状态机

归属：`SystemSupervisor`

```text
PowerOn
  -> BoardInit
  -> PlatformInit
  -> StorageMount
  -> LoadSettings
  -> InitDisplay
  -> InitOutput
  -> Ready
  -> Fault
```

状态说明：

- `PowerOn`：复位后入口。
- `BoardInit`：时钟、引脚、基础外设。
- `PlatformInit`：队列、线程、日志、shell。
- `StorageMount`：挂载 Flash 文件系统。
- `LoadSettings`：读取配置和校准。
- `InitDisplay`：初始化屏幕。
- `InitOutput`：准备 DAC/TIM/DMA。
- `Ready`：可接受用户命令。
- `Fault`：不可恢复或需用户确认的故障。

### 10.2 信号业务状态机

归属：`SignalEngine`

```text
Idle
  -> Editing
  -> Arming
  -> Running
  -> Paused
  -> Stopping
  -> Fault
```

主要迁移：

| 当前状态 | 事件 | 条件 | 下一个状态 |
| --- | --- | --- | --- |
| `Idle` | `EditProfile` | 参数合法 | `Editing` |
| `Editing` | `ApplyProfile` | 校验通过 | `Idle` |
| `Idle` | `Start` | profile 合法且输出可用 | `Arming` |
| `Arming` | `OutputReady` | buffer 足够 | `Running` |
| `Running` | `Pause` | 支持暂停 | `Paused` |
| `Paused` | `Resume` | buffer 足够 | `Running` |
| `Running` | `Stop` | - | `Stopping` |
| `Stopping` | `OutputStopped` | - | `Idle` |
| 任意 | `FaultDetected` | - | `Fault` |
| `Fault` | `ClearFault` | 故障已解除 | `Idle` |

### 10.3 输出状态机

归属：`OutputScheduler`

```text
Unconfigured
  -> Configured
  -> Priming
  -> Streaming
  -> Draining
  -> Stopped
  -> Underrun
  -> Fault
```

关键规则：

- `Configured`：采样率、幅度、offset 已校验并下发。
- `Priming`：预填充 DMA buffer 和 ready queue。
- `Streaming`：DMA 正在运行。
- `Draining`：收到 stop 后停止补充新数据，等待 DMA 安全停止。
- `Underrun`：输出数据不足，可选择静音、保持最后值或停止。
- `Fault`：DMA error、DAC error、非法配置。

### 10.4 存储状态机

归属：`WaveformRepository`

```text
Unmounted
  -> Mounting
  -> Ready
  -> Busy
  -> Recovering
  -> Fault
```

规则：

- 文件写入时必须有临时文件和原子提交策略。
- 波形索引损坏时进入 `Recovering`。
- 输出实时路径不能等待 `Busy` 状态的文件操作。

### 10.5 UI 状态机

归属：`UiPresenter` 和页面对象。

```text
BootScreen
  -> MainScreen
  -> EditSignalScreen
  -> WaveformBrowserScreen
  -> StorageScreen
  -> SystemScreen
  -> FaultScreen
```

UI 状态不能直接改变输出硬件，只能生成 `SignalCommand`。

## 11. 命令和事件模型

建议统一命令和事件类型，避免模块之间直接调用过深。

```cpp
enum class SignalCommandKind {
    start,
    stop,
    pause,
    resume,
    set_frequency,
    set_amplitude,
    set_offset,
    set_waveform,
    save_preset,
    load_preset,
    clear_fault,
};

struct SignalCommand {
    SignalCommandKind kind;
    uint32_t source;
    CommandPayload payload;
    uint32_t sequence;
};
```

```cpp
enum class SystemEventKind {
    output_started,
    output_stopped,
    output_underrun,
    storage_mounted,
    storage_fault,
    input_received,
    profile_changed,
    fault_detected,
};

struct SystemEvent {
    SystemEventKind kind;
    uint32_t timestamp_ms;
    EventPayload payload;
};
```

命令表示“请求做什么”，事件表示“已经发生什么”。不要混用。

## 12. Snapshot 设计

每个重要服务提供只读快照：

```cpp
struct SignalEngineSnapshot {
    SignalEngineState state;
    SignalProfile active_profile;
    WaveformId selected_waveform;
    FaultCode last_fault;
    uint32_t command_count;
};

struct OutputSnapshot {
    OutputState state;
    SampleRateHz sample_rate;
    uint32_t queued_blocks;
    uint32_t underrun_count;
    uint32_t dma_error_count;
};

struct StorageSnapshot {
    RepositoryState state;
    uint32_t waveform_count;
    uint32_t last_op_duration_ms;
    ErrorCode last_error;
};

struct SystemSnapshot {
    SignalEngineSnapshot signal;
    OutputSnapshot output;
    StorageSnapshot storage;
    InputSnapshot input;
    UiSnapshot ui;
};
```

Snapshot 原则：

- 外部只能拿拷贝。
- Snapshot 不持有锁时间过长。
- UI 和 shell 不访问服务内部可变对象。
- 高频指标使用 atomic 或单写单读结构。

## 13. Zephyr 配置和 DTS 规划

当前 DTS 已启用 USART1、DMA、时钟和 LED。后续应逐步补充：

### 13.1 存储

建议描述：

- QSPI/OSPI/SPI NOR 控制器。
- W25Q64 flash 节点。
- fixed partitions。
- LittleFS 分区。

示意：

```dts
&ospi1 {
    status = "okay";
    pinctrl-0 = <...>;
    pinctrl-names = "default";

    w25q64: flash@0 {
        compatible = "jedec,spi-nor";
        reg = <0>;
        size = <DT_SIZE_M(64)>;
        spi-max-frequency = <80000000>;
    };
};

&w25q64 {
    partitions {
        compatible = "fixed-partitions";
        #address-cells = <1>;
        #size-cells = <1>;

        waveform_partition: partition@0 {
            label = "waveforms";
            reg = <0x00000000 0x00600000>;
        };

        settings_partition: partition@600000 {
            label = "settings";
            reg = <0x00600000 0x00100000>;
        };
    };
};
```

实际节点名称、总线和 pinctrl 需要按原理图和 Zephyr STM32H7 支持情况确认。

### 13.2 显示

需要建模：

- FMC 8080 bus。
- panel 节点。
- reset/backlight/tearing effect 引脚。

若 Zephyr 没有可直接复用的 STM32 FMC 8080 display bus，需要新增自定义 binding 和 driver。

### 13.3 输出

需要建模：

- DAC 通道。
- 触发 TIM。
- DMA 通道。
- 可选模拟开关/输出使能 GPIO。

如果 Zephyr 标准 DAC API 不满足连续 DMA 输出，建议实现自定义 `dac-wave-sink` driver，并在 platform 层包装成 `WaveSinkPort`。

### 13.4 输入

需要建模：

- GPIO keys。
- encoder A/B 引脚。
- 可选 encoder 按键。

优先接入 Zephyr input subsystem；上层统一看到 `InputEvent`。

### 13.5 prj.conf 建议方向

后续可能需要：

```text
CONFIG_LOG=y
CONFIG_SHELL=y
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y
CONFIG_FILE_SYSTEM=y
CONFIG_FILE_SYSTEM_LITTLEFS=y
CONFIG_INPUT=y
CONFIG_GPIO=y
CONFIG_DMA=y
CONFIG_DAC=y
CONFIG_COUNTER=y
CONFIG_CPP=y
```

具体配置应随模块引入逐步打开，避免一次性打开大量未使用能力。

## 14. 构建步骤建议

建议按以下阶段落地，不要一次性完成全部架构。

### 阶段 1：工程骨架

- 将 `src/main.c` 迁移为 `src/app/main.cpp`。
- 新增 `include/omnigen/base`、`domain`、`ports`。
- CMake 按目录组织源文件。
- 启用 C++ 编译，确认 Zephyr 构建通过。
- 建立 `composition_root.cpp`，但暂时只启动最小 demo。

### 阶段 2：基础类型和状态快照

- 实现 `Result`、`ErrorCode`、单位类型。
- 定义 `SignalProfile`、`SignalLimits`、`OutputState`。
- 实现 `SignalEngineSnapshot`、`OutputSnapshot`。
- 编写 native unit test 验证 profile 校验。

### 阶段 3：输入和控制流

- 接入按键/编码器。
- 实现 `InputEvent`、`InputRouter`、`SignalCommand`。
- 用 shell 或 UART 模拟命令。
- 验证状态机从 `Idle` 到 `Editing`、`Running` 的逻辑。

### 阶段 4：输出链路

- 实现 `WaveSinkPort`。
- 先用 mock wave sink 验证 `OutputScheduler`。
- 再接入 DAC/TIM/DMA。
- 验证固定频率正弦/方波输出。
- 增加 underrun 计数和 DMA error 处理。

### 阶段 5：存储链路

- 接入 W25Q64 和 LittleFS。
- 实现 `StoragePort`。
- 实现 `WaveformRepository`。
- 验证波形文件枚举、保存、读取。
- 输出线程从预读 buffer 获取数据，不直接读文件。

### 阶段 6：显示和 UI

- 接入 FMC 8080 屏幕。
- 实现 `DisplayPort`。
- 建立 `UiPresenter`。
- 用 snapshot 渲染主界面和参数编辑界面。

### 阶段 7：诊断和稳定性

- 增加 shell 命令：`status`、`output start/stop`、`wave list`、`fault clear`。
- 增加 metrics。
- 增加 watchdog 策略。
- 增加硬件 smoke test。

## 15. CMake 组织建议

目标是让 CMake 反映目录职责：

```cmake
target_include_directories(app PRIVATE
  include
)

target_sources(app PRIVATE
  src/app/main.cpp
  src/app/composition_root.cpp
  src/domain/waveform_synthesis.cpp
  src/domain/signal_validation.cpp
  src/services/signal_engine.cpp
  src/services/output_scheduler.cpp
  src/services/waveform_repository.cpp
  src/services/input_router.cpp
  src/platform/zephyr/zephyr_time.cpp
  src/platform/zephyr/zephyr_storage.cpp
  src/platform/zephyr/zephyr_wave_sink.cpp
  src/diagnostics/shell_commands.cpp
)

zephyr_library_sources_ifdef(CONFIG_OMNIGEN_DAC_WAVE_SINK
  drivers/output/dac_wave_sink.c
)
```

模块多起来后，可在每个目录建立局部 `CMakeLists.txt`，但早期不要过度拆分。

## 16. 命名和文件规则

建议规则：

- 文件名使用 `snake_case`。
- C++ 类型使用 `PascalCase`。
- 函数和变量使用 `snake_case`。
- 硬件驱动 C 文件遵循 Zephyr 风格。
- 所有公开头文件放 `include/omnigen/...`。
- 只被单个 `.cpp` 使用的类型放源文件内部。
- ISR callback 名称带 `_isr` 或 `_callback`。
- 状态枚举带模块前缀，例如 `OutputState`、`StorageState`。

## 17. 风险点和约束

### 17.1 STM32H723 cache 与 DMA

Cortex-M7 cache 会影响 DMA buffer 一致性。DAC DMA、FMC、QSPI 相关 buffer 必须明确：

- 是否放在 non-cacheable 区域。
- 是否需要 cache clean/invalidate。
- 是否满足 DMA 对齐。
- 谁拥有 buffer 的写权限。

### 17.2 输出实时性

采样率越高，输出线程留给填充 buffer 的时间越少。必须提前定义：

- `kSamplesPerBlock`
- `kBlockCount`
- 最大采样率。
- 单次合成耗时上限。
- 文件读取预取策略。

### 17.3 文件系统阻塞

LittleFS 操作可能阻塞，不能出现在输出实时线程中。任意波输出应通过 storage thread 预读。

### 17.4 屏幕刷新带宽

480x320 RGB565 全屏约 300 KiB。FMC 刷新应避免频繁全屏刷新。UI 需要局部刷新和 dirty rect。

### 17.5 过度抽象

不是所有设备都需要 port。只有上层需要稳定能力边界、需要 mock、可能替换或存在多个实现时才定义 port。

## 18. 推荐最小第一版

第一版不必一次完成所有模块。建议最小闭环：

```text
SignalProfile
  -> SignalEngine
  -> OutputScheduler
  -> MockWaveSink or DACWaveSink
  -> Shell command start/stop
  -> OutputSnapshot
```

对应最少文件：

```text
include/omnigen/base/error.hpp
include/omnigen/base/result.hpp
include/omnigen/base/units.hpp
include/omnigen/domain/signal_profile.hpp
include/omnigen/ports/wave_sink_port.hpp
src/app/main.cpp
src/app/composition_root.cpp
src/domain/signal_validation.cpp
src/services/signal_engine.cpp
src/services/output_scheduler.cpp
src/platform/zephyr/zephyr_wave_sink.cpp
src/diagnostics/shell_commands.cpp
```

完成这个闭环后，再加入输入、存储、显示。这样架构能在真实代码中被验证，而不是停留在目录设计。
