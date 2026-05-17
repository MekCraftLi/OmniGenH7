/**
 *******************************************************************************
 * @file    signal_snapshot.hpp
 * @brief   Immutable state snapshots for diagnostics and UI
 *******************************************************************************
 * @attention
 *
 * Snapshots provide read-only copies of internal state for:
 * - UI rendering (no direct access to service internals)
 * - Shell/diagnostics output
 * - Host protocol queries
 *
 * External modules should only access state through snapshots,
 * never by reading service internal variables directly.
 *
 *******************************************************************************
 * @note
 *
 * Snapshots are copied, not referenced. This ensures:
 * - No lock contention during UI rendering
 * - Immutable data for safe concurrent access
 * - Clear ownership boundaries
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/05
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "signal_profile.hpp"
#include "signal_state.hpp"
#include "base/result.hpp"

#include <cstdint>

namespace omnigen {

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 波形存储标识符。
 *
 * 用于在预设、任意波形或外部存储中引用某个波形记录。该结构只保存逻辑 ID，
 * 不携带存储地址或文件系统细节。
 */
struct WaveformId {
    uint32_t value; /**< 波形记录逻辑编号。 */
};

/**
 * @brief 信号引擎状态快照。
 *
 * 保存业务状态机和当前有效配置的只读副本，供 UI、Shell 和诊断接口读取。
 * 快照是值拷贝，不暴露 `SignalEngine` 内部可变成员。
 */
struct SignalEngineSnapshot {
    SignalEngineState state;       /**< 当前业务状态。 */
    SignalProfile active_profile;  /**< 当前生效的信号配置。 */
    WaveformId selected_waveform;   /**< 当前选中的波形标识。 */
    ErrorCode last_fault;           /**< 最近一次故障错误码。 */
    uint32_t command_count;         /**< 已处理命令数量。 */
};

/**
 * @brief 输出调度状态快照。
 *
 * 保存输出硬件链路的状态和关键统计计数，主要用于诊断性能、DMA 错误和缓冲区
 * 欠载问题。
 */
struct OutputSnapshot {
    OutputState state;         /**< 当前输出链路状态。 */
    SampleRateHz sample_rate;  /**< 当前输出采样率。 */
    uint32_t queued_blocks;    /**< 已排队等待输出的数据块数量。 */
    uint32_t underrun_count;   /**< 输出欠载累计次数。 */
    uint32_t dma_error_count;  /**< DMA 错误累计次数。 */
};

/**
 * @brief 存储子系统状态快照。
 *
 * 保存外部存储或波形仓库的概览状态，用于判断存储是否可用、记录数量以及最近
 * 一次操作结果。
 */
struct StorageSnapshot {
    bool mounted;                /**< 存储设备是否已挂载或初始化成功。 */
    uint32_t waveform_count;     /**< 已知波形记录数量。 */
    uint32_t last_op_duration_ms; /**< 最近一次存储操作耗时，单位 ms。 */
    ErrorCode last_error;        /**< 最近一次存储操作错误码。 */
};

/**
 * @brief 系统组合快照。
 *
 * 将信号引擎、输出链路和存储子系统的快照组合为一个诊断视图，适合一次性输出
 * 到 Shell、UI 或主机协议。
 */
struct SystemSnapshot {
    SignalEngineSnapshot signal; /**< 信号引擎快照。 */
    OutputSnapshot output;       /**< 输出链路快照。 */
    StorageSnapshot storage;     /**< 存储子系统快照。 */
};

} // namespace omnigen