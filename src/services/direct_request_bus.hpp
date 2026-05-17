/**
 *******************************************************************************
 * @file    direct_request_bus.hpp
 * @brief   Direct in-process implementation of RequestBusPort
 *******************************************************************************
 * @attention
 *
 * DirectRequestBus collects read-side snapshots and statuses synchronously from
 * current service and port instances.
 *
 *******************************************************************************
 * @note
 *
 * This implementation does not cache responses; each request reads current state.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "ports/display_port.hpp"
#include "ports/filter_switch_port.hpp"
#include "ports/request_bus_port.hpp"
#include "ports/storage_port.hpp"
#include "services/signal_engine.hpp"

namespace omnigen {

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

class DirectRequestBus : public RequestBusPort {
public:
    DirectRequestBus(SignalEngine& signal_engine, FilterSwitchPort& filter_switch, StoragePort& storage,
                     DisplayPort& display, bool& system_ready);
    ~DirectRequestBus() override = default;

    Result<void> request(const AppRequest& request, AppResponse& response) override;

private:
    SignalEngine& signal_engine_;
    FilterSwitchPort& filter_switch_;
    StoragePort& storage_;
    DisplayPort& display_;
    bool& system_ready_;
};

} // namespace omnigen
