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

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "base/result.hpp"
#include "domain/signal_profile.hpp"

#include <cstdint>
#include <stddef.h>

namespace omnigen {

/*-------- 2. data structures ----------------------------------------------------------------------------------------*/

struct WaveSampleBlock {
    const uint16_t* samples;
    size_t count;
    uint32_t sample_rate_hz;
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief Abstract interface for signal output.
 */
class WaveSinkPort {
public:
    virtual ~WaveSinkPort() = default;

    /**
     * @brief Configure the output with given profile.
     */
    virtual Result<void> configure(const SignalProfile& profile) = 0;

    /**
     * @brief Start signal output.
     */
    virtual Result<void> start() = 0;

    /**
     * @brief Stop signal output.
     */
    virtual Result<void> stop() = 0;

    /**
     * @brief Submit samples for DMA output.
     * @param block Sample block descriptor.
     */
    virtual Result<void> submit_block(const WaveSampleBlock& block) = 0;
};

} // namespace omnigen
