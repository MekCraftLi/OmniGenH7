/**
 *******************************************************************************
 * @file    mock_wave_sink.hpp
 * @brief   Mock wave sink for testing without hardware
 *******************************************************************************
 * @attention
 *
 * Mock implementation of WaveSinkPort for development and testing.
 * Does not output to real hardware, just tracks state.
 *
 *******************************************************************************
 * @note
 *
 * Use this for shell testing and unit tests.
 * Replace with real DAC implementation in production.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "ports/wave_sink_port.hpp"

namespace omnigen {

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief Mock implementation of WaveSinkPort.
 */
class MockWaveSink : public WaveSinkPort {
public:
    MockWaveSink() = default;

    Result<void> configure(const SignalProfile& profile) override;
    Result<void> start() override;
    Result<void> stop() override;
    Result<void> submit_block(const WaveSampleBlock& block) override;

    /* Test helpers */
    bool is_running() const { return running_; }
    const SignalProfile& configured_profile() const { return profile_; }
    size_t blocks_submitted() const { return blocks_submitted_; }

private:
    SignalProfile profile_{};
    bool running_{false};
    bool configured_{false};
    size_t blocks_submitted_{0};
};

} // namespace omnigen
