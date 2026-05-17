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

/**
 * @brief 波形样本块描述符。
 *
 * 该结构只描述一段待输出样本的地址、长度和采样率，不拥有样本内存。调用方必须
 * 保证 `samples` 在提交期间有效。
 */
struct WaveSampleBlock {
    const uint16_t* samples;  /**< 12 位 DAC 样本数组首地址。 */
    size_t count;             /**< 样本数量。 */
    uint32_t sample_rate_hz;  /**< 样本块对应的采样率，单位 Hz。 */
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 波形输出端口抽象类。
 *
 * 该接口隔离信号引擎与具体输出硬件。实现类可以是 DAC+TIM+DMA、外部 DAC 或
 * 测试用 mock，但必须遵循统一的配置、启动、停止和样本提交语义。
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
