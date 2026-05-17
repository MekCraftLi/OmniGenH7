/**
 *******************************************************************************
 * @file    signal_command.hpp
 * @brief   Signal command definitions for control flow
 *******************************************************************************
 * @attention
 *
 * Commands represent "what to do" requests from user input or host protocol.
 * Commands are processed by SignalEngine state machine.
 *
 *******************************************************************************
 * @note
 *
 * Command flow:
 * Button/Encoder/Host -> InputEvent -> InputRouter -> SignalCommand -> SignalEngine
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "signal_profile.hpp"
#include "base/result.hpp"

#include <cstdint>

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

/**
 * @brief 信号控制命令类型枚举。
 *
 * 表达外部输入、Shell 或内部逻辑希望信号引擎执行的动作。具体参数保存在
 * `SignalCommand` 中，由 `kind` 决定哪些字段有意义。
 */
enum class SignalCommandKind : uint8_t {
    None,         /**< 空命令或未初始化命令。 */
    Start,        /**< 启动输出。 */
    Stop,         /**< 停止输出。 */
    Pause,        /**< 暂停输出。 */
    Resume,       /**< 恢复输出。 */
    SetFrequency, /**< 设置频率。 */
    SetAmplitude, /**< 设置幅值。 */
    SetOffset,    /**< 设置直流偏置。 */
    SetWaveform,  /**< 设置波形类型。 */
    SetDuty,      /**< 设置占空比。 */
    LoadPreset,   /**< 加载预设配置。 */
    SavePreset,   /**< 保存预设配置。 */
    ClearFault,   /**< 清除故障状态。 */
};

/**
 * @brief 命令来源枚举。
 *
 * 标识命令由哪个入口产生，用于诊断、权限策略和后续输入路由扩展。来源只描述
 * 入口，不改变命令本身的业务语义。
 */
enum class CommandSource : uint8_t {
    None,     /**< 未指定来源。 */
    Button,   /**< 板载按键。 */
    Encoder,  /**< 旋钮编码器。 */
    HostUart, /**< 主机 UART 协议。 */
    Shell,    /**< Zephyr Shell 命令。 */
    Internal, /**< 系统内部生成。 */
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 信号引擎命令结构体。
 *
 * 统一承载写入类控制意图。该结构使用简单字段而非 union，便于调试和 Shell
 * 构造；调用方应根据 `kind` 填写对应参数字段。
 */
struct SignalCommand {
    SignalCommandKind kind; /**< 命令类型。 */
    CommandSource source;   /**< 命令来源。 */
    uint32_t sequence;      /**< 单调递增命令序号，用于追踪命令顺序。 */

    FrequencyHz frequency;  /**< `SetFrequency` 使用的目标频率。 */
    VoltageMv amplitude;    /**< `SetAmplitude` 使用的目标幅值。 */
    VoltageMv offset;       /**< `SetOffset` 使用的目标直流偏置。 */
    WaveformKind waveform;  /**< `SetWaveform` 使用的波形类型。 */
    DutyPermille duty;      /**< `SetDuty` 使用的占空比。 */
    uint32_t preset_id;     /**< 预设加载或保存使用的标识符。 */

    static SignalCommand make_start(CommandSource src);
    static SignalCommand make_stop(CommandSource src);
    static SignalCommand make_pause(CommandSource src);
    static SignalCommand make_resume(CommandSource src);
    static SignalCommand make_set_frequency(CommandSource src, FrequencyHz freq);
    static SignalCommand make_set_amplitude(CommandSource src, VoltageMv amp);
    static SignalCommand make_set_waveform(CommandSource src, WaveformKind kind);
    static SignalCommand make_clear_fault(CommandSource src);
};

} // namespace omnigen