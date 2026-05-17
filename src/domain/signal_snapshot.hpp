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
 * @brief Waveform identifier for storage lookup.
 */
struct WaveformId {
    uint32_t value;
};

/**
 * @brief Snapshot of signal engine state.
 *
 * Captures the main business state machine and active configuration.
 */
struct SignalEngineSnapshot {
    SignalEngineState state;
    SignalProfile active_profile;
    WaveformId selected_waveform;
    ErrorCode last_fault;
    uint32_t command_count;
};

/**
 * @brief Snapshot of output scheduler state.
 *
 * Captures the output hardware state and performance metrics.
 */
struct OutputSnapshot {
    OutputState state;
    SampleRateHz sample_rate;
    uint32_t queued_blocks;
    uint32_t underrun_count;
    uint32_t dma_error_count;
};

/**
 * @brief Snapshot of storage/repository state.
 */
struct StorageSnapshot {
    bool mounted;
    uint32_t waveform_count;
    uint32_t last_op_duration_ms;
    ErrorCode last_error;
};

/**
 * @brief Combined system snapshot for diagnostics.
 */
struct SystemSnapshot {
    SignalEngineSnapshot signal;
    OutputSnapshot output;
    StorageSnapshot storage;
};

} // namespace omnigen