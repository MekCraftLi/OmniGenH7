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

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "ports/wave_sink_port.hpp"
#include "domain/waveform_synthesis.hpp"

#include <zephyr/kernel.h>

namespace omnigen {

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief Zephyr DAC 波形输出适配类。
 *
 * 该类把领域层的 `WaveSinkPort` 调用转换为板级 DAC 波形输出驱动操作，负责维护
 * 当前配置、运行状态和本地样本缓冲区。
 */
class ZephyrWaveSink : public WaveSinkPort {
public:
    ZephyrWaveSink();
    ~ZephyrWaveSink() override = default;

    Result<void> configure(const SignalProfile& profile) override;
    Result<void> start() override;
    Result<void> stop() override;
    Result<void> submit_block(const WaveSampleBlock& block) override;

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
