/**
 *******************************************************************************
 * @file    mock_wave_sink.cpp
 * @brief   Mock wave sink implementation
 *******************************************************************************
 * @attention
 *
 * Simple mock that tracks state without hardware output.
 * Useful for testing SignalEngine without DAC hardware.
 *
 *******************************************************************************
 * @note
 *
 * This mock always succeeds. For error testing, create
 * a separate FailingMockWaveSink.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "platform/mock_wave_sink.hpp"

namespace omnigen {

/*-------- 3. implementation -----------------------------------------------------------------------------------------*/

Result<void> MockWaveSink::configure(const SignalProfile& profile)
{
    profile_ = profile;
    configured_ = true;
    return ErrorCode::Ok;
}

Result<void> MockWaveSink::start()
{
    if (!configured_) {
        return ErrorCode::InvalidState;
    }
    running_ = true;
    return ErrorCode::Ok;
}

Result<void> MockWaveSink::stop()
{
    running_ = false;
    return ErrorCode::Ok;
}

Result<void> MockWaveSink::submit_block(const WaveSampleBlock& block)
{
    (void)block;
    blocks_submitted_++;
    return ErrorCode::Ok;
}

} // namespace omnigen
