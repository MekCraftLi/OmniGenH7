/**
 *******************************************************************************
 * @file    dac_control_manager.hpp
 * @brief   DAC control manager thread and WaveSinkPort implementation
 *******************************************************************************
 */

#ifndef OMNIGEN_H7_APP_PLATFORM_ZEPHYR_DAC_CONTROL_MANAGER_HPP
#define OMNIGEN_H7_APP_PLATFORM_ZEPHYR_DAC_CONTROL_MANAGER_HPP

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "base/singleton.hpp"
#include "platform/zephyr/application_base.h"
#include "ports/wave_sink_port.hpp"

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/sys/atomic.h>

namespace omnigen {

/* ------- class -----------------------------------------------------------------------------------------------------*/

class DacControlManager final : public NotifyApp, public WaveSinkPort, public Singleton<DacControlManager> {
    friend class Singleton<DacControlManager>;

public:
    DacControlManager(const DacControlManager&) = delete;
    DacControlManager& operator=(const DacControlManager&) = delete;

    Result<void> configure(const SignalProfile& profile) override;
    Result<void> start() override;
    Result<void> stop() override;
    Result<void> submit_block(const uint16_t* samples, size_t count) override;

    [[nodiscard]] bool is_ready() const;
    [[nodiscard]] bool is_running() const;
    [[nodiscard]] uint32_t run_count() const;

protected:
    void init() override;
    void run() override;

private:
    static void on_dma_complete(const struct device* dev, void* user_data, uint32_t channel, int status);
    Result<void> apply_runtime_profile();
    Result<void> rebuild_wave_buffer();
    uint16_t calc_sine_sample(uint32_t phase_index) const;
    uint16_t calc_triangle_sample(uint32_t phase_index) const;
    uint16_t calc_saw_sample(uint32_t phase_index) const;
    uint16_t calc_square_sample(uint32_t phase_index) const;
    [[nodiscard]] uint32_t active_wave_table_size() const;
    uint16_t convert_mv_to_dac_code(int32_t mv) const;
    int32_t calc_level_mv_from_norm(int32_t norm_milli) const;
    Result<void> start_dma();
    void stop_dma();

    static constexpr uint8_t kDacChannelId     = 1U;
    static constexpr uint8_t kDacResolutionBit = 12U;
    static constexpr uint32_t kDacFullScale    = (1U << kDacResolutionBit) - 1U;
    static constexpr uint32_t kWaveTableSize   = 64U;
    static constexpr uint32_t kDefaultFreqHz   = 1000U;
    static constexpr uint32_t kDacVrefMv       = 3300U;

    DacControlManager();

    const struct device* dac_dev_{nullptr};
    const struct device* tim6_counter_dev_{nullptr};
    const struct device* dma_dev_{nullptr};
    struct dac_channel_cfg dac_cfg_{};
    struct counter_top_cfg tim6_top_cfg_{};
    struct dma_config dma_cfg_{};
    struct dma_block_config dma_block_cfg_{};
    uint32_t dma_channel_{0U};
    bool dma_configured_{false};

    SignalProfile profile_{};
    atomic_t running_{0};
    bool ready_{false};
    bool configured_{false};
    uint32_t run_count_{0U};
    uint32_t timer_input_hz_{0U};
    uint32_t sample_rate_hz_{kDefaultFreqHz * kWaveTableSize};
    uint16_t wave_table_size_{kWaveTableSize};
    uint32_t tim6_top_ticks_{0U};
    uint32_t dma_error_count_{0U};
    uint16_t wave_buffer_[kWaveTableSize]{};
};

} // namespace omnigen

#endif /* OMNIGEN_H7_APP_PLATFORM_ZEPHYR_DAC_CONTROL_MANAGER_HPP */
