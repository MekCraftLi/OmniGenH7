/**
 *******************************************************************************
 * @file    zephyr_wave_sink.cpp
 * @brief   Zephyr implementation of WaveSinkPort using DAC
 *******************************************************************************
 * @attention
 *
 * Implements WaveSinkPort interface using the DAC wave sink driver.
 * Generates waveform samples and outputs them via DAC.
 *
 *******************************************************************************
 * @note
 *
 * Currently uses MockWaveSink behavior until DAC driver is fully integrated.
 * The DAC driver initialization and output will be added in the next step.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "platform/zephyr_wave_sink.hpp"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zephyr_wave_sink, CONFIG_LOG_DEFAULT_LEVEL);

namespace omnigen {

/* ------- function implement ----------------------------------------------------------------------------------------*/

ZephyrWaveSink::ZephyrWaveSink()
    : running_(false)
    , configured_(false)
{
    /* Initialize sample buffer to mid-scale */
    for (int i = 0; i < 256; i++) {
        sample_buffer_[i] = 2048;
    }
}

Result<void> ZephyrWaveSink::configure(const SignalProfile& profile)
{
    LOG_INF("Configuring wave sink:");
    LOG_INF("  Waveform: %d", static_cast<int>(profile.kind));
    LOG_INF("  Frequency: %u Hz", profile.frequency.value);
    LOG_INF("  Sample rate: %u Hz", profile.sample_rate.value);
    LOG_INF("  Amplitude: %d mV", profile.amplitude.value);
    LOG_INF("  Offset: %d mV", profile.offset.value);

    /* Store profile */
    profile_ = profile;

    /* Generate one cycle of waveform samples */
    generate_waveform(sample_buffer_, 256, profile_);

    configured_ = true;
    return ErrorCode::Ok;
}

Result<void> ZephyrWaveSink::start()
{
    if (!configured_) {
        LOG_ERR("Wave sink not configured");
        return ErrorCode::InvalidState;
    }

    if (running_) {
        return ErrorCode::Ok;
    }

    LOG_INF("Starting DAC output");

    /* TODO: Call DAC driver to start output */
    /* dac_wave_sink_start(dac_dev); */

    running_ = true;
    return ErrorCode::Ok;
}

Result<void> ZephyrWaveSink::stop()
{
    if (!running_) {
        return ErrorCode::Ok;
    }

    LOG_INF("Stopping DAC output");

    /* TODO: Call DAC driver to stop output */
    /* dac_wave_sink_stop(dac_dev); */

    running_ = false;
    return ErrorCode::Ok;
}

Result<void> ZephyrWaveSink::submit_block(const uint16_t* samples, size_t count)
{
    if (!running_) {
        return ErrorCode::InvalidState;
    }

    /* TODO: Submit samples to DAC DMA buffer */
    /* dac_wave_sink_set_buffer(dac_dev, samples, count); */

    (void)samples;
    (void)count;

    return ErrorCode::Ok;
}

} // namespace omnigen
