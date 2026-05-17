/**
 *******************************************************************************
 * @file    request_bus_port.hpp
 * @brief   Application request bus boundary
 *******************************************************************************
 * @attention
 *
 * RequestBusPort is the read-side application boundary for typed status and
 * snapshot queries.
 *
 *******************************************************************************
 * @note
 *
 * Request/response data must remain small and copyable. Large data reads belong
 * in dedicated ports such as StoragePort.
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
#include "domain/system_status.hpp"

namespace omnigen {

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

class RequestBusPort {
public:
    virtual ~RequestBusPort() = default;

    virtual Result<void> request(const AppRequest& request, AppResponse& response) = 0;
};

} // namespace omnigen
