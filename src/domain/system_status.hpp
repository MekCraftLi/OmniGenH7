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

/**
 * @brief 应用读请求类型枚举。
 *
 * 描述 `RequestBusPort` 支持的查询类别。每个枚举值对应 `AppResponse` 中的一组
 * 有效响应字段。
 */
enum class AppRequestKind : uint8_t {
    None,           /**< 空请求或未初始化请求。 */
    SignalSnapshot, /**< 查询信号引擎详细快照。 */
    FilterStatus,   /**< 查询滤波器切换模块状态。 */
    StorageStatus,  /**< 查询存储模块状态。 */
    DisplayStatus,  /**< 查询显示模块状态。 */
    SystemStatus,   /**< 查询系统整体就绪状态。 */
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 滤波器切换模块状态。
 *
 * 描述模拟滤波器硬件是否初始化、当前是否静音以及当前选择的滤波档位。
 */
struct FilterStatus {
    bool mounted;    /**< 滤波器 GPIO 控制是否已初始化。 */
    bool muted;      /**< 输出是否处于静音状态。 */
    FilterMode mode; /**< 当前滤波器档位。 */
};

/**
 * @brief 存储模块状态。
 *
 * 用于通过请求总线快速判断外部 NOR Flash 或存储适配器是否可用。
 */
struct StorageStatus {
    bool mounted; /**< 存储设备是否已初始化并可访问。 */
};

/**
 * @brief 显示模块状态。
 *
 * 用于通过请求总线快速判断 LCD 显示适配器是否完成初始化。
 */
struct DisplayStatus {
    bool mounted; /**< 显示设备是否已初始化并可绘制。 */
};

/**
 * @brief 系统整体状态。
 *
 * 表示组合根初始化流程是否完成。该状态不代表每个可选硬件都成功初始化，只表示
 * 应用主框架已经进入可响应命令的阶段。
 */
struct SystemStatus {
    bool ready; /**< 系统是否已完成启动流程。 */
};

/**
 * @brief 应用读请求结构体。
 *
 * 由 Shell、UI 或主机协议创建后提交给 `RequestBusPort`。请求本身只表达“要读
 * 什么”，不携带返回数据。
 */
struct AppRequest {
    AppRequestKind kind{AppRequestKind::None}; /**< 请求类型。 */
    uint32_t sequence{0U};                     /**< 请求序号，用于追踪查询顺序。 */

    static AppRequest make(AppRequestKind kind);
};

/**
 * @brief 应用读响应结构体。
 *
 * 作为请求总线统一响应信封使用。`kind` 和 `status` 描述本次查询结果，具体有效
 * 数据由请求类型决定。
 */
struct AppResponse {
    AppRequestKind kind{AppRequestKind::None}; /**< 响应对应的请求类型。 */
    ErrorCode status{ErrorCode::Ok};           /**< 查询执行结果。 */
    SignalEngineSnapshot signal{};             /**< 信号引擎快照响应。 */
    FilterStatus filter{};                     /**< 滤波器状态响应。 */
    StorageStatus storage{};                   /**< 存储状态响应。 */
    DisplayStatus display{};                   /**< 显示状态响应。 */
    SystemStatus system{};                     /**< 系统状态响应。 */
};

} // namespace omnigen
