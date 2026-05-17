/**
 *******************************************************************************
 * @file    signal_state.hpp
 * @brief   Signal engine and output state enumerations
 *******************************************************************************
 * @attention
 *
 * State enumerations for signal engine and output scheduler.
 * These states drive the main state machines in the system.
 *
 *******************************************************************************
 * @note
 *
 * SignalEngineState: Main business state machine (Idle -> Running -> etc.)
 * OutputState: Output hardware state machine (Configured -> Streaming -> etc.)
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/05
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cstdint>

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

/**
 * @brief 信号引擎业务状态枚举。
 *
 * 描述用户命令和业务逻辑层看到的主状态机状态。该状态机负责表达“是否正在
 * 编辑、准备输出、运行、暂停或故障”，不直接描述底层 DMA/DAC 的细节。
 *
 * 典型转换：
 * - Idle -> Editing：用户正在编辑参数
 * - Idle -> Arming：用户请求启动输出
 * - Arming -> Running：输出硬件准备完成
 * - Running -> Paused：用户暂停输出
 * - Paused -> Running：用户恢复输出
 * - Running -> Stopping：用户请求停止
 * - Stopping -> Idle：输出完全停止
 * - Any -> Fault：检测到错误
 */
enum class SignalEngineState : uint8_t {
    Idle,     /**< 空闲状态，未输出信号。 */
    Editing,  /**< 参数编辑状态。 */
    Arming,   /**< 启动准备状态。 */
    Running,  /**< 正在输出信号。 */
    Paused,   /**< 输出暂停但配置仍保留。 */
    Stopping, /**< 停止流程进行中。 */
    Fault,    /**< 故障状态，需要清除错误后恢复。 */
};

/**
 * @brief 输出调度和硬件输出状态枚举。
 *
 * 描述输出链路靠近硬件的一侧状态，重点关注配置、缓冲、DMA 流式输出、排空和
 * 欠载等细节。业务层通常只通过快照读取该状态。
 *
 * 典型转换：
 * - Unconfigured -> Configured：已设置输出参数
 * - Configured -> Priming：正在填充初始缓冲区
 * - Priming -> Streaming：DMA 正在连续输出
 * - Streaming -> Draining：收到停止请求并等待排空
 * - Draining -> Stopped：输出完全停止
 * - Streaming -> Underrun：缓冲区欠载
 * - Any -> Fault：DMA/DAC 错误
 */
enum class OutputState : uint8_t {
    Unconfigured, /**< 尚未配置输出参数。 */
    Configured,   /**< 已配置但尚未开始输出。 */
    Priming,      /**< 正在准备初始样本数据。 */
    Streaming,    /**< 正在通过硬件连续输出。 */
    Draining,     /**< 正在停止并排空输出链路。 */
    Stopped,      /**< 输出已停止。 */
    Underrun,     /**< 样本供应不足导致欠载。 */
    Fault,        /**< 输出硬件或调度器故障。 */
};

} // namespace omnigen