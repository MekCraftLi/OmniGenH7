/**
 *******************************************************************************
 * @file    zephyr_wave_sink.hpp
 * @brief   Zephyr implementation of WaveSinkPort using DAC
 *******************************************************************************
 * @attention
 *
 * Implements WaveSinkPort interface using the board DAC wave sink driver.
 * Connects the domain layer to hardware output.
 *
 *******************************************************************************
 * @note
 *
 * The Zephyr device lifetime is owned by the kernel device model. This adapter
 * owns only the local waveform staging buffer copied into the driver DMA buffer.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.1
 *******************************************************************************
 */
#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "domain/waveform_synthesis.hpp"
#include "ports/wave_sink_port.hpp"

#include <cstddef>
#include <cstdint>

struct device;

namespace omnigen {

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief Zephyr DAC 波形输出适配类。
 *
 * 该类把领域层的 `WaveSinkPort` 调用转换为板级 DAC 波形输出驱动操作。对象不创建线程，
 * 不拥有 Zephyr 设备实例；它只持有当前配置、运行状态和本地样本暂存缓冲区。所有控制调用
 * 为同步调用，调用方应通过命令总线保持串行访问。
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
    static constexpr size_t k_sample_count = 512U;

    const struct device* dac_dev_;
    SignalProfile profile_{};
    bool running_;
    bool configured_;
    uint16_t sample_buffer_[k_sample_count];
};

} // namespace omnigen
