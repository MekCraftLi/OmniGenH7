/**
 *******************************************************************************
 * @file    zephyr_wave_sink.cpp
 * @brief   Zephyr implementation of WaveSinkPort using DAC
 *******************************************************************************
 * @attention
 *
 * Implements the WaveSinkPort interface using the board-level DAC wave sink
 * device.
 *
 *******************************************************************************
 * @note
 *
 * The adapter copies generated samples into the driver-owned DMA buffer before
 * output starts; it does not keep caller-provided sample pointers.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.1
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "platform/zephyr_wave_sink.hpp"

#include "drivers/dac_wave_sink.h"

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zephyr_wave_sink, CONFIG_LOG_DEFAULT_LEVEL);

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

#define DAC_WAVE_SINK_NODE DT_NODELABEL(dac_wave_sink)

namespace omnigen {

/*-------- 3. internal helpers ---------------------------------------------------------------------------------------*/

static Result<void> map_errno_to_result(int ret)
{
    if (ret == 0) {
        return ErrorCode::Ok;
    }

    switch (ret) {
        case -EINVAL:
            return ErrorCode::InvalidArgument;
        case -ENODEV:
            return ErrorCode::InvalidState;
        case -EBUSY:
            return ErrorCode::Busy;
        case -ETIMEDOUT:
            return ErrorCode::Timeout;
        default:
            return ErrorCode::IoError;
    }
}

/*-------- 4. implementation -----------------------------------------------------------------------------------------*/

ZephyrWaveSink::ZephyrWaveSink()
    : dac_dev_(DEVICE_DT_GET(DAC_WAVE_SINK_NODE))
    , running_(false)
    , configured_(false)
{
    for (size_t i = 0U; i < k_sample_count; ++i) {
        sample_buffer_[i] = kDacMid;
    }
}

Result<void> ZephyrWaveSink::configure(const SignalProfile& profile)
{
    if (!device_is_ready(dac_dev_)) {
        LOG_ERR("DAC wave sink device not ready");
        return ErrorCode::InvalidState;
    }

    if (running_) {
        return ErrorCode::Busy;
    }

    if (profile.sample_rate.value == 0U) {
        return ErrorCode::InvalidArgument;
    }

    profile_ = profile;
    generate_waveform(sample_buffer_, k_sample_count, profile_);

    int ret = dac_wave_sink_configure(dac_dev_, profile_.sample_rate.value);
    if (ret != 0) {
        LOG_ERR("DAC wave sink configure failed: %d", ret);
        return map_errno_to_result(ret);
    }

    ret = dac_wave_sink_set_buffer(dac_dev_, sample_buffer_, k_sample_count);
    if (ret != 0) {
        LOG_ERR("DAC wave sink buffer update failed: %d", ret);
        return map_errno_to_result(ret);
    }

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

    int ret = dac_wave_sink_start(dac_dev_);
    if (ret != 0) {
        LOG_ERR("DAC wave sink start failed: %d", ret);
        return map_errno_to_result(ret);
    }

    running_ = true;
    return ErrorCode::Ok;
}

Result<void> ZephyrWaveSink::stop()
{
    if (!running_) {
        return ErrorCode::Ok;
    }

    int ret = dac_wave_sink_stop(dac_dev_);
    if (ret != 0) {
        LOG_ERR("DAC wave sink stop failed: %d", ret);
        return map_errno_to_result(ret);
    }

    running_ = false;
    return ErrorCode::Ok;
}

Result<void> ZephyrWaveSink::submit_block(const WaveSampleBlock& block)
{
    if (block.samples == nullptr || block.count == 0U) {
        return ErrorCode::InvalidArgument;
    }

    int ret = dac_wave_sink_set_buffer(dac_dev_, block.samples, block.count);
    if (ret != 0) {
        LOG_ERR("DAC wave sink submit failed: %d", ret);
        return map_errno_to_result(ret);
    }

    return ErrorCode::Ok;
}

} // namespace omnigen
