/**
 *******************************************************************************
 * @file    wave_sink_port.hpp
 * @brief   Wave sink port interface for signal output
 *******************************************************************************
 * @attention
 *
 * Abstract interface for signal output. Implementations may use
 * DAC+TIM+DMA, external DAC, or other hardware mechanisms.
 *
 *******************************************************************************
 * @note
 *
 * The WaveSinkPort interface hides hardware details from the
 * SignalEngine and OutputScheduler services.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/05
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "base/result.hpp"
#include "domain/signal_profile.hpp"

#include <cstdint>
#include <span>

namespace omnigen {

/* ------- class prototypes ------------------------------------------------------------------------------------------*/

/**
 * @brief Abstract interface for signal output.
 */
class WaveSinkPort {
public:
    virtual ~WaveSinkPort() = default;

    /**
     * @brief Configure the output with given profile.
     */
    [[nodiscard]] virtual Result<void> configure(const SignalProfile& profile) = 0;

    /**
     * @brief Start signal output.
     */
    [[nodiscard]] virtual Result<void> start() = 0;

    /**
     * @brief Stop signal output.
     */
    [[nodiscard]] virtual Result<void> stop() = 0;

    /**
     * @brief Submit samples for DMA output.
     */
    [[nodiscard]] virtual Result<void> submit_block(std::span<const uint16_t> samples) = 0;
};

} // namespace omnigen
