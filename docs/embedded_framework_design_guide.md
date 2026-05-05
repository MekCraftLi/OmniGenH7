# Embedded Framework Design Guide

本文档整理 OmniGenH7 项目的工程分层、文件组织、资源管理、数据流和代码规范建议。当前实验背景是基于 STM32H723ZG 的数字信号发生器，但本文档的目标是沉淀一套可复用的嵌入式工程框架设计准则。

## 1. 设计目标

框架目标不是简单增加目录层级，而是给系统建立清晰的边界：

- 明确定义每一层的职责、依赖方向和允许暴露的信息。
- 让硬件驱动、业务逻辑、平台服务、调试接口相互解耦。
- 让资源申请、释放、所有权、并发访问方式可追踪。
- 让全局数据流动从隐式共享变量转为显式命令、事件、状态快照。
- 让调试时可以观察系统结构化状态，而不破坏模块封装。
- 让同一套思想可以落地在 Zephyr、STM32Cube HAL + FreeRTOS 或裸机框架上。

核心原则：

```text
下层说硬件语言，上层说业务语言，中间层只负责翻译和编排。
```

如果一个文件里同时大量出现寄存器、DMA、GPIO、中断细节和业务概念，通常说明层级边界已经混乱。

## 2. 分类标准

分类应从大到小逐步确定，不要先按个人习惯创建目录。

### 2.1 按依赖方向分类

第一层分类应服务于依赖方向：

```text
app
 -> services
 -> ports
 <- drivers
 -> platform
 -> board
 -> vendor_hal / rtos / third_party
```

建议规则：

- `domain` 不依赖硬件、不依赖 RTOS、不依赖 HAL。
- `services` 依赖 `domain` 和 `ports`，负责业务编排和状态机。
- `ports` 定义上层需要的能力接口。
- `drivers` 实现具体设备或硬件能力。
- `platform` 封装 RTOS、HAL、文件系统、时间、锁、队列等平台细节。
- `board` 描述具体板级硬件事实和中断入口路由。
- `third_party` 保存外部库，不在其中直接写业务代码。

### 2.2 按变化原因分类

第二层分类应考虑修改原因：

- 换芯片会变：`board/`、`platform/stm32_hal/`
- 换 RTOS 会变：`platform/freertos/`、`platform/zephyr/`
- 换 IMU 型号会变：`drivers/imu/`
- 换通信协议会变：`middleware/protocol/`
- 换业务规则会变：`domain/`、`services/`
- 换显示页面会变：`ui/`
- 换调试方式会变：`diagnostics/`

修改原因不同的代码不应强行放在同一文件中。

### 2.3 按语言边界分类

第三层分类应避免语言混杂：

- 硬件语言：寄存器、DMA、IRQ、GPIO、CAN frame、UART byte stream。
- 能力语言：`MotorPort`、`StoragePort`、`SerialPort`、`WaveSink`。
- 业务语言：目标速度、波形、校准、输出状态、用户命令、故障策略。

模块应尽量只使用同一种语言。需要跨语言时，应通过 adapter 或 port 完成。

## 3. 通用文件结构

推荐通用结构如下：

```text
project/
  app/
    main.cpp
    composition_root.cpp

  board/
    stm32h723/
      clock.c
      gpio.c
      msp.c
      irq_routes.c
      pinmap.hpp

  platform/
    stm32_hal/
      can_hal_port.cpp
      uart_hal_port.cpp
      spi_hal_port.cpp
      i2c_hal_port.cpp
      gpio_interrupt.cpp
      timer_hal_port.cpp

    freertos/
      thread.cpp
      queue.cpp
      event_group.cpp
      mutex.cpp
      time.cpp

    zephyr/
      device_ref.hpp
      littlefs_storage.cpp
      settings_store.cpp
      input_source.cpp
      wave_sink.cpp

  ports/
    motor_port.hpp
    imu_port.hpp
    serial_port.hpp
    storage_port.hpp
    display_port.hpp
    wave_sink_port.hpp
    time_port.hpp

  drivers/
    motor/
      can_motor_driver.cpp
      can_motor_driver.hpp

    imu/
      bmi088_driver.cpp
      bmi088_driver.hpp
      imu_interrupt_handler.cpp

    display/
    storage/
    encoder/
    button/

  domain/
    motor_command.hpp
    imu_sample.hpp
    waveform.hpp
    signal_profile.hpp
    calibration.hpp
    protocol_frame.hpp

  services/
    motor_service.cpp
    imu_service.cpp
    host_protocol_service.cpp
    signal_output_service.cpp
    waveform_repository.cpp
    system_state_service.cpp

  middleware/
    protocol/
    filters/
    math/
    file_format/

  ui/
    screens/
    widgets/

  diagnostics/
    log.cpp
    shell.cpp
    metrics.cpp
    state_snapshot.cpp

  generated/
  third_party/
  tests/
  docs/
```

`composition_root.cpp` 是应用装配根，负责创建对象、绑定依赖、注入接口、启动线程。其他模块不应到处创建全局服务。

## 4. Zephyr 落地结构

在 Zephyr 项目中，应把 Zephyr 看成操作系统和硬件抽象底座，不要并行重造一套大 HAL。

```text
OmniGenH7/
  CMakeLists.txt
  Kconfig
  prj.conf
  west.yml

  boards/arm/omnigen_h7/
    omnigen_h7.dts
    omnigen_h7_defconfig
    board.cmake
    Kconfig.board
    pinctrl.dtsi

  dts/bindings/
    display/your,fmc-8080-dbi.yaml
    output/your,dac-wave-sink.yaml

  drivers/
    display/fmc_8080_dbi.c
    display/panel_xxx.c
    output/dac_wave_sink.c

  include/omnigen/
    base/result.hpp
    base/units.hpp
    base/error.hpp
    ports/storage.hpp
    ports/display.hpp
    ports/input.hpp
    ports/wave_sink.hpp
    domain/waveform.hpp

  src/
    app/main.cpp
    app/composition_root.cpp
    platform/zephyr/
    domain/
    services/
    ui/
    diagnostics/

  tests/
  docs/
```

Zephyr 中的职责建议：

- Devicetree 描述硬件事实：总线、引脚、时钟、中断、Flash 分区、设备实例。
- Kconfig 描述软件能力：是否启用模块、缓冲区大小、日志等级、可选特性。
- Zephyr driver 使用 C 实现，接入 Zephyr device model。
- 应用业务层可使用 C++，但不应让 Zephyr 头文件污染 `domain`。
- 外部 Flash 建议使用 fixed partitions + flash map + LittleFS。
- 输入设备优先使用 Zephyr input/GPIO/QDEC 能力。
- 屏幕优先接入 Zephyr display API；FMC/8080 并口可作为 bus/transport adapter。
- DAC 信号输出应抽象为 `WaveSink`，内部可由 DAC + TIM + DMA 实现。

## 5. STM32Cube HAL + FreeRTOS 落地结构

脱离 Zephyr 时，HAL 和 FreeRTOS 应被限制在 `platform/`、`board/` 和部分底层 `drivers/` 中。

推荐依赖关系：

```text
services -> ports <- drivers -> platform -> board -> STM32Cube HAL / FreeRTOS
```

CubeMX 生成代码建议放入 `generated/` 或受控的 `board/` 子目录。业务代码不要直接写进容易被 CubeMX 覆盖的区域。确实需要使用 USER CODE 区域时，只做最薄的入口转发。

## 6. 设备、外设与模块归属

建议使用以下判断：

- MCU 外设：属于芯片内部硬件能力，例如 GPIO、UART、SPI、I2C、CAN、FMC、QSPI/OSPI、DAC、TIM、DMA。
- 外部设备：通过总线连接的器件，例如 W25Q64、屏幕控制器、IMU、编码器、CAN 电机、按键组。
- 应用能力：由多个外设和设备组合出来的系统能力，例如数字信号输出、波形文件管理、电机控制、姿态估计、上位机协议。

示例：

- `FMC` 是 MCU 外设。
- 8080 并口屏幕控制器是外部设备。
- `DAC + TIM + DMA` 是底层输出机制。
- `SignalOutputService` 是应用能力。
- `CAN` 是 MCU 外设和通信总线。
- `CAN motor driver` 是具体设备驱动。
- `MotorService` 是业务编排和状态机。

## 7. 驱动职责

设备驱动不只是信息打包和拆包。驱动应负责：

- 初始化和硬件能力探测。
- 时序约束、总线事务、寄存器访问。
- 中断底半部通知或内部 worker。
- DMA、cache、buffer 对齐和并发保护。
- 错误检测、错误码转换、基础恢复。
- 设备状态和健康状态查询。
- 功耗状态切换，若项目需要。
- 将硬件细节转换为稳定的能力接口。

驱动不应负责：

- UI 策略。
- 文件格式策略。
- 高层业务状态机。
- 用户命令解释。
- 跨模块全局状态管理。

## 8. 接口设计规则

接口不是所有设备的必需品。接口应服务于隔离变化，而不是服务于形式上的面向对象。

需要接口的情况：

- 上层不应该知道具体型号。
- 存在多个实现。
- 需要单元测试 mock。
- 设备或平台未来可能替换。
- 上层依赖的是能力，而不是某个具体硬件。

不需要接口的情况：

- 项目中只有一个实现。
- 没有测试替身需求。
- 上层本来就必须依赖具体硬件能力。
- 抽象后只剩空泛的 `init/read/write`。

接口建议放在 `ports/`：

```cpp
class MotorPort {
public:
    virtual ~MotorPort() = default;

    [[nodiscard]] virtual Result<void> set_target(MotorCommand command) = 0;
    [[nodiscard]] virtual Result<MotorState> read_state() = 0;
    [[nodiscard]] virtual MotorHealth health() const = 0;
};
```

服务依赖接口，驱动实现接口：

```text
services/motor_service.cpp -> ports/motor_port.hpp <- drivers/motor/can_motor_driver.cpp
```

不建议建立巨大的万能 `IDevice`：

```cpp
class IDevice {
public:
    virtual init();
    virtual open();
    virtual close();
    virtual read();
    virtual write();
    virtual reset();
    virtual suspend();
    virtual resume();
};
```

更好的方法是按能力拆分，或者直接为具体能力定义明确接口：

```cpp
struct Resettable {
    virtual Result<void> reset() = 0;
};

struct HealthCheckable {
    virtual DeviceHealth health() const = 0;
};
```

但在嵌入式 C++ 中，继承应谨慎使用。优先让接口足够少、足够稳定、足够有业务含义。

## 9. 中断与回调归属

HAL callback 或 IRQ handler 不应承载业务逻辑。

推荐链路：

```text
HAL IRQ / callback
  -> board/platform 中断分发
  -> driver 的 ISR-safe notify
  -> FreeRTOS task / service 处理
```

示例：

```cpp
extern "C" void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    GpioInterruptDispatcher::on_exti(pin);
}
```

```cpp
void GpioInterruptDispatcher::on_exti(uint16_t pin)
{
    if (pin == IMU_INT1_Pin) {
        ImuInterruptHandler::on_data_ready_isr();
    }
}
```

```cpp
void ImuInterruptHandler::on_data_ready_isr()
{
    BaseType_t woke = pdFALSE;
    xTaskNotifyFromISR(imu_task_handle, IMU_DATA_READY, eSetBits, &woke);
    portYIELD_FROM_ISR(woke);
}
```

真正读取 IMU 应放在线程上下文：

```text
EXTI IRQ
  -> notify imu task
  -> imu service reads sample through driver
  -> filter service consumes sample
  -> state snapshot is updated
```

CAN 和 UART 同理：

```text
CAN Rx IRQ
  -> can_hal_port
  -> can_rx_queue
  -> motor driver
  -> motor service
```

```text
UART DMA idle IRQ
  -> uart_hal_port
  -> rx ring buffer
  -> host protocol service
```

中断入口可以集中，归属逻辑应分散到对应驱动或服务。不要把所有业务写入一个全局 `interrupt.c`，也不要让线程文件直接塞满 HAL callback。

## 10. 线程与任务归属

线程应按职责归属，而不是机械按外设归属。

判断规则：

- 线程处理业务状态机：放 `services/`。
- 线程只是维护设备收发：放 `drivers/`。
- 线程只是封装 RTOS 能力：放 `platform/`。

示例：

```text
services/
  motor_service.cpp
  imu_service.cpp
  host_protocol_service.cpp
  signal_output_service.cpp

drivers/
  imu/bmi088_worker.cpp
  display/panel_refresh_worker.cpp
```

每个主动模块建议拥有：

- 明确的输入队列或 task notification。
- 明确的状态机。
- 明确的状态快照。
- 明确的启动、停止和错误处理策略。

## 11. 全局数据流动

复杂系统中的全局数据不要直接通过全局变量共享。应将数据流分为三类。

### 11.1 控制流

控制流表示命令、事件和状态切换：

```text
button / encoder / host protocol
  -> command bus
  -> service
  -> domain state transition
```

适合使用：

- FreeRTOS queue
- Zephyr message queue
- event flags
- command bus
- active object

### 11.2 数据流

数据流表示高速、连续、实时数据：

```text
file / decoder
  -> block buffer
  -> ring buffer
  -> DMA
  -> DAC
```

适合使用：

- ring buffer
- double buffer
- DMA descriptor
- fixed block allocator
- lock-free single producer/single consumer queue

实时数据流中应避免：

- heap 分配。
- 文件 IO。
- 阻塞锁。
- 复杂日志。
- UI 调用。

### 11.3 状态流

状态流用于 UI、shell、日志和调试：

```text
service internal state
  -> immutable snapshot
  -> ui / diagnostics / host query
```

外部模块不应为了显示或调试直接读取服务内部变量。每个关键服务应提供结构化 `status()` 或 `snapshot()`。

## 12. 资源管理

资源应有明确所有权和生命周期。

资源类型包括：

- HAL handle、Zephyr device、bus handle。
- FreeRTOS task、queue、mutex、event group、timer。
- DMA buffer、ring buffer、block pool。
- 文件句柄、filesystem mount、Flash 分区。
- 校准参数、配置参数、运行状态。
- 中断订阅、回调注册。
- 日志模块、shell 命令、调试计数器。

建议规则：

- C++ 中使用 RAII 管理文件、锁、会话、buffer 租约。
- Zephyr `struct device` 或 HAL 静态 handle 通常由平台生命周期管理，C++ 只持有非拥有引用。
- 所有可能失败的资源申请返回 `Result<T, Error>`。
- 实时路径不做动态资源申请。
- 所有跨线程共享状态必须说明同步机制。
- 全局对象只用于不可变配置、平台句柄或装配根创建的服务引用。

示例：

```cpp
class FileHandle {
public:
    FileHandle(StoragePort& storage, FileId id);
    ~FileHandle();

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    FileHandle(FileHandle&&) noexcept = default;
    FileHandle& operator=(FileHandle&&) noexcept = default;
};
```

## 13. 当前数字信号发生器建议模块

当前项目可按如下方式映射：

```text
domain/
  waveform.hpp
  signal_profile.hpp
  calibration.hpp

ports/
  storage_port.hpp
  wave_sink_port.hpp
  display_port.hpp
  input_port.hpp

drivers/
  storage/w25q64_driver.cpp
  display/fmc_8080_bus.cpp
  display/panel_driver.cpp
  output/dac_dma_wave_sink.cpp
  input/button_driver.cpp
  input/encoder_driver.cpp

services/
  waveform_repository.cpp
  signal_engine.cpp
  output_scheduler.cpp
  input_router.cpp
  ui_presenter.cpp

diagnostics/
  signal_debug_shell.cpp
  output_metrics.cpp
  system_snapshot.cpp
```

推荐数据链路：

```text
W25Q64 / filesystem
  -> waveform repository
  -> waveform decoder
  -> output scheduler
  -> DMA buffer
  -> TIM + DAC
```

推荐控制链路：

```text
buttons / encoder / host protocol
  -> input router
  -> command bus
  -> signal engine
  -> output scheduler / waveform repository / ui presenter
```

推荐状态链路：

```text
signal engine / output scheduler / storage
  -> snapshots
  -> display / shell / host query
```

## 14. 现代 C++ 使用准则

建议使用现代 C++ 的语义表达能力，但保持嵌入式可控性。

推荐：

- `enum class` 表达强类型状态和错误。
- `std::span` 表达非拥有连续缓冲区。
- `std::array` 表达固定容量。
- `std::string_view` 表达非拥有字符串。
- `std::optional` 表达可缺省值。
- `Result<T, Error>` 表达可失败操作。
- `[[nodiscard]]` 标注必须处理的结果。
- 强类型单位，例如 `FrequencyHz`、`VoltageMv`、`SampleRateHz`。
- RAII 管理文件、锁、buffer 租约、输出会话。

谨慎或禁用：

- 异常。
- RTTI。
- 实时路径动态分配。
- 复杂模板元编程。
- 隐式全局单例。
- 构造函数中执行复杂硬件初始化。

建议命名：

- 类型：`PascalCase`
- 函数和变量：`snake_case`
- 常量：`kPascalCase` 或项目统一风格
- C 接口和 HAL callback：遵循平台既有命名
- 单位写入类型名或变量名：`sample_rate_hz`、`timeout_ms`

## 15. Rust 引入建议

Rust 可以引入，但不建议一开始用于底层驱动和中断路径。

适合 Rust 的模块：

- 波形文件解析。
- 协议帧解析。
- 参数校验。
- CRC 和编码解码。
- host 端可 fuzz 的纯逻辑。

建议方式：

- Rust 编译为 static library。
- 通过 C ABI 暴露少量稳定函数。
- C++ `platform` 或 `middleware` 层封装 Rust FFI。
- 不让 Rust FFI 直接扩散到业务服务各处。

不建议初期使用 Rust 的区域：

- HAL callback。
- DMA ISR。
- FreeRTOS task glue。
- Zephyr driver。
- 需要频繁访问厂商 HAL 的代码。

## 16. 设计模式

适合本框架的模式：

- Hexagonal Architecture：`ports/` 定义能力，`platform/` 和 `drivers/` 实现能力。
- Active Object：每个有线程的 service 用队列或通知驱动。
- Repository：文件、配置、校准数据通过 repository 访问。
- Strategy：波形生成器、滤波器、插值器、输出模式可替换。
- State Machine：输出状态、UI 状态、文件写入、通信连接状态。
- Snapshot：调试变量和 UI 状态统一快照化。
- RAII：资源申请和释放绑定对象生命周期。
- Adapter：隔离 Zephyr、HAL、FreeRTOS、第三方库接口。

不建议滥用：

- 万能总线。
- 万能 `Device` 基类。
- 到处可访问的 service locator。
- 巨大的全局 `SystemContext`。
- 过度继承层级。

## 17. 文件注释模板

### 17.1 头文件模板

```cpp
/**
 * @file waveform_repository.hpp
 * @brief Persistent access to waveform records.
 *
 * @responsibility
 * Enumerate, load, save, and delete waveform records.
 *
 * @threading
 * Called from application context. Not ISR-safe.
 *
 * @ownership
 * Does not own the storage backend. The caller must keep StoragePort alive.
 *
 * @error_model
 * Returns Result<T, Error>. No exceptions are thrown.
 */

#pragma once

/* Includes */

/* Public constants */

/* Public types */

/* Public interfaces */

/* Inline helpers */
```

### 17.2 源文件模板

```cpp
/**
 * @file waveform_repository.cpp
 * @brief Implementation of waveform persistent storage.
 */

/* Includes */

/* Internal constants */

/* Internal types */

/* Private helpers */

/* Public implementation */

/* Diagnostics hooks */
```

### 17.3 C 驱动文件模板

```c
/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file dac_wave_sink.c
 * @brief DMA-backed DAC wave output driver.
 */

/* Includes */

/* Register and timing constants */

/* Device configuration */

/* Runtime data */

/* ISR handlers */

/* Driver API implementation */

/* Initialization */
```

## 18. 模块设计检查清单

新增模块前应回答：

- 这个模块说的是硬件语言、能力语言还是业务语言？
- 它的上层调用者是谁？
- 它允许依赖哪些目录？
- 它拥有哪些资源？
- 它是否会被中断调用？
- 它是否会被多个线程调用？
- 它是否需要 mock 或替换实现？
- 它暴露的是实时数据、控制命令还是状态快照？
- 它的错误是否可恢复？
- 它的状态如何被诊断工具观察？

如果这些问题答不清楚，先不要创建新目录或新抽象。

## 19. 总结规则

- 先按依赖方向分类，再按变化原因细分，最后按设备或功能命名文件。
- 中断入口只做分发，ISR 只做通知，业务逻辑在线程上下文处理。
- 驱动负责硬件可靠工作，服务负责业务状态机。
- 接口只用于隔离真实变化，不要求所有设备都有接口。
- 不做万能 `IDevice`，按能力定义小而稳定的 port。
- 全局数据流用 command、event、snapshot 显式表达。
- 实时数据路径避免 heap、阻塞锁、文件 IO 和复杂日志。
- 调试观察走状态快照，不破坏封装。
- 平台代码适配 HAL/RTOS，业务代码不直接依赖 HAL/RTOS。
- 架构文档和代码结构应随项目演进持续维护。
