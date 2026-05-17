/**
 *******************************************************************************
 * @file    system_status.hpp
 * @brief   Unified system status and query data types
 *******************************************************************************
 * @attention
 *
 * This file defines read-side data structures shared by request bus users and
 * system services.
 *
 *******************************************************************************
 * @note
 *
 * Naming convention:
 * - Command: write intent
 * - Request: read/query intent
 * - Response: read/query result envelope
 * - Status: current module state
 * - Snapshot: detailed service state copy
 * - Payload/Block: large data descriptor
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
#include "domain/signal_snapshot.hpp"
#include "ports/filter_switch_port.hpp"

#include <cstdint>

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

enum class AppRequestKind : uint8_t {
    None,
    SignalSnapshot,
    FilterStatus,
    StorageStatus,
    DisplayStatus,
    SystemStatus,
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

struct FilterStatus {
    bool mounted;
    bool muted;
    FilterMode mode;
};

struct StorageStatus {
    bool mounted;
};

struct DisplayStatus {
    bool mounted;
};

struct SystemStatus {
    bool ready;
};

struct AppRequest {
    AppRequestKind kind{AppRequestKind::None};
    uint32_t sequence{0U};

    static AppRequest make(AppRequestKind kind);
};

struct AppResponse {
    AppRequestKind kind{AppRequestKind::None};
    ErrorCode status{ErrorCode::Ok};
    SignalEngineSnapshot signal{};
    FilterStatus filter{};
    StorageStatus storage{};
    DisplayStatus display{};
    SystemStatus system{};
};

} // namespace omnigen
