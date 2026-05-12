/**
 *******************************************************************************
 * @file    zephyr_wave_sink.hpp
 * @brief   Zephyr implementation of WaveSinkPort using DAC
 *******************************************************************************
 * @attention
 *
 * Implements WaveSinkPort interface using the DAC wave sink driver.
 * Connects the domain layer to hardware output.
 *
 *******************************************************************************
 * @note
 *
 * This implementation uses the dac_wave_sink driver for actual output.
 * For testing without hardware, use MockWaveSink instead.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "ports/wave_sink_port.hpp"
#include "domain/waveform_synthesis.hpp"

#include <zephyr/kernel.h>

namespace omnigen {

/* ------- class prototypes ------------------------------------------------------------------------------------------*/

/**
 * @brief Zephyr implementation of WaveSinkPort.
 *
 * Uses DAC + TIM + DMA for continuous waveform output.
 */
class ZephyrWaveSink : public WaveSinkPort {
public:
    ZephyrWaveSink();
    ~ZephyrWaveSink() override = default;

    Result<void> configure(const SignalProfile& profile) override;
    Result<void> start() override;
    Result<void> stop() override;
    Result<void> submit_block(const uint16_t* samples, size_t count) override;

    /**
     * @brief Check if output is currently running.
     */
    bool is_running() const { return running_; }

private:
    SignalProfile profile_;
    bool running_;
    bool configured_;
    uint16_t sample_buffer_[256];
};

} // namespace omnigen
