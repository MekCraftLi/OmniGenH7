/**
 *******************************************************************************
 * @file    command_bus_port.hpp
 * @brief   Application command bus boundary
 *******************************************************************************
 * @attention
 *
 * CommandBusPort is the write-side application boundary for user, shell, host,
 * and input commands.
 *
 *******************************************************************************
 * @note
 *
 * The bus carries small typed commands only. Large payloads must use dedicated
 * ports such as StoragePort, DisplayPort, or WaveSinkPort.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "base/result.hpp"
#include "domain/signal_command.hpp"
#include "ports/filter_switch_port.hpp"

#include <cstdint>

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

/**
 * @brief 应用写命令类别枚举。
 *
 * 用于区分 `AppCommand` 中当前携带的是哪一类写入意图。该枚举是命令总线的
 * 顶层路由依据。
 */
enum class AppCommandKind : uint8_t {
    None,   /**< 空命令或未初始化命令。 */
    Signal, /**< 信号引擎控制命令。 */
    Filter, /**< 滤波器切换控制命令。 */
};

/**
 * @brief 滤波器控制命令类别枚举。
 *
 * 用于描述滤波器端口支持的写操作，包括切换档位和设置静音状态。
 */
enum class FilterCommandKind : uint8_t {
    None,    /**< 空命令或未初始化命令。 */
    SetMode, /**< 设置滤波器档位。 */
    SetMute, /**< 设置输出静音状态。 */
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 滤波器控制命令结构体。
 *
 * 统一承载对滤波器硬件适配器的写入意图。根据 `kind` 的不同，`mode` 或 `mute`
 * 字段会被命令总线转发给 `FilterSwitchPort`。
 */
struct FilterCommand {
    FilterCommandKind kind{FilterCommandKind::None}; /**< 滤波器命令类型。 */
    CommandSource source{CommandSource::None};       /**< 命令来源。 */
    uint32_t sequence{0U};                           /**< 命令序号。 */
    FilterMode mode{FilterMode::Bypass};             /**< `SetMode` 使用的目标滤波档位。 */
    bool mute{false};                                /**< `SetMute` 使用的目标静音状态。 */

    static FilterCommand make_set_mode(CommandSource source, FilterMode mode);
    static FilterCommand make_set_mute(CommandSource source, bool enabled);
};

/**
 * @brief 应用写命令信封结构体。
 *
 * 作为 `CommandBusPort` 的统一输入类型，包裹不同子系统的命令。`kind` 决定当前
 * 有效的是 `signal` 还是 `filter` 字段。
 */
struct AppCommand {
    AppCommandKind kind{AppCommandKind::None}; /**< 应用命令类别。 */
    SignalCommand signal{};                    /**< 信号引擎命令载荷。 */
    FilterCommand filter{};                    /**< 滤波器命令载荷。 */

    static AppCommand make_signal(const SignalCommand& command);
    static AppCommand make_filter(const FilterCommand& command);
};

/**
 * @brief 应用写命令总线端口抽象类。
 *
 * 该接口是 Shell、UI、主机协议等写入入口到业务服务之间的边界。实现类负责
 * 根据 `AppCommandKind` 同步或异步分发命令。
 */
class CommandBusPort {
public:
    virtual ~CommandBusPort() = default;

    virtual Result<void> submit(const AppCommand& command) = 0;
};

} // namespace omnigen
