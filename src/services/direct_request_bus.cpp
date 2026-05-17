/**
 *******************************************************************************
 * @file    direct_request_bus.cpp
 * @brief   Direct in-process implementation of RequestBusPort
 *******************************************************************************
 * @attention
 *
 * This file dispatches read-side requests to the current service and port graph.
 *
 *******************************************************************************
 * @note
 *
 * Responses are filled by the caller-provided output object to avoid dynamic
 * allocation and large return values.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "services/direct_request_bus.hpp"

namespace omnigen {

/*-------- 3. implementation -----------------------------------------------------------------------------------------*/

DirectRequestBus::DirectRequestBus(SignalEngine& signal_engine, FilterSwitchPort& filter_switch, StoragePort& storage,
                                   DisplayPort& display, bool& system_ready)
    : signal_engine_(signal_engine)
    , filter_switch_(filter_switch)
    , storage_(storage)
    , display_(display)
    , system_ready_(system_ready)
{
}

Result<void> DirectRequestBus::request(const AppRequest& request, AppResponse& response)
{
    response = AppResponse{};
    response.kind = request.kind;
    response.status = ErrorCode::Ok;

    switch (request.kind) {
        case AppRequestKind::SignalSnapshot:
            response.signal = signal_engine_.snapshot();
            return ErrorCode::Ok;

        case AppRequestKind::FilterStatus:
            response.filter = FilterStatus{
                .mounted = filter_switch_.mounted(),
                .muted = filter_switch_.muted(),
                .mode = filter_switch_.mode(),
            };
            return ErrorCode::Ok;

        case AppRequestKind::StorageStatus:
            response.storage = StorageStatus{
                .mounted = storage_.mounted(),
            };
            return ErrorCode::Ok;

        case AppRequestKind::DisplayStatus:
            response.display = DisplayStatus{
                .mounted = display_.mounted(),
            };
            return ErrorCode::Ok;

        case AppRequestKind::SystemStatus:
            response.system = SystemStatus{
                .ready = system_ready_,
            };
            return ErrorCode::Ok;

        default:
            response.status = ErrorCode::InvalidArgument;
            return ErrorCode::InvalidArgument;
    }
}

} // namespace omnigen
