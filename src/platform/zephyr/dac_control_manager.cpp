/**
 *******************************************************************************
 * @file    dac_control_manager.cpp
 * @brief   DAC control manager thread implementation
 *******************************************************************************
 */

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "platform/zephyr/dac_control_manager.hpp"

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree/dma.h>
#include <zephyr/cache.h>

#include <soc.h>
#include <stm32_ll_dac.h>
#include <stm32_ll_tim.h>
#include <math.h>

LOG_MODULE_REGISTER(dac_control_manager, CONFIG_LOG_DEFAULT_LEVEL);

namespace omnigen {

/* ------- variables -------------------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true
#define APPLICATION_NAME       "dac_ctrl"
#define APPLICATION_STACK_SIZE 1536
#define APPLICATION_PRIORITY   6

K_THREAD_STACK_DEFINE(g_dac_ctrl_stack, APPLICATION_STACK_SIZE);

[[maybe_unused]] static auto& forceInit = DacControlManager::instance();

static_assert(DT_NODE_HAS_STATUS(DT_NODELABEL(dac1), okay), "dac1 must be enabled in board dts");
static_assert(DT_DMAS_HAS_NAME(DT_NODELABEL(dac1), tx), "dac1 must define dmas and dma-names = \"tx\"");
static_assert(DT_NODE_EXISTS(DT_NODELABEL(timers6)), "timers6 node must exist");
static_assert(DT_NODE_HAS_STATUS(DT_CHILD(DT_NODELABEL(timers6), counter), okay),
              "timers6 counter child must be enabled in board dts");

static inline TIM_TypeDef* tim6_regs() {
    return reinterpret_cast<TIM_TypeDef*>(DT_REG_ADDR(DT_NODELABEL(timers6)));
}

static inline void configure_tim6_trgo_update() {
    LL_TIM_SetTriggerOutput(tim6_regs(), LL_TIM_TRGO_UPDATE);
}

static constexpr float kTwoPi = 6.28318530718F;

static inline void flush_wave_buffer_cache(uint16_t* buf, size_t size_bytes) {
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
    int rc = sys_cache_data_flush_range(buf, size_bytes);
    if (rc != 0 && rc != -ENOTSUP) {
        LOG_WRN("D-cache flush failed for waveform buffer: %d", rc);
    }
#else
    ARG_UNUSED(buf);
    ARG_UNUSED(size_bytes);
#endif
}

/* ------- function implement ----------------------------------------------------------------------------------------*/

DacControlManager::DacControlManager()
    : NotifyApp(APPLICATION_ENABLE, APPLICATION_NAME, K_THREAD_STACK_SIZEOF(g_dac_ctrl_stack), g_dac_ctrl_stack,
                APPLICATION_PRIORITY, K_MSEC(5)) {
}

Result<void> DacControlManager::configure(const SignalProfile& profile) {
    profile_ = profile;
    configured_ = true;

    Result<void> rc = apply_runtime_profile();
    if (rc.is_error()) {
        return rc.error();
    }

    LOG_INF("DAC configured: waveform=%d freq=%uHz sample_rate=%uHz", static_cast<int>(profile_.kind),
            profile_.frequency.value, sample_rate_hz_);
    return ErrorCode::Ok;
}

Result<void> DacControlManager::start() {
    if (!ready_) {
        return ErrorCode::InvalidState;
    }

    if (!configured_) {
        return ErrorCode::InvalidState;
    }

    Result<void> dma_rc = start_dma();
    if (dma_rc.is_error()) {
        return dma_rc.error();
    }

    int rc = counter_start(tim6_counter_dev_);
    if (rc != 0) {
        LOG_ERR("TIM6 start failed: %d", rc);
        stop_dma();
        return ErrorCode::HardwareFault;
    }

    atomic_set(&running_, 1);

    LOG_INF("DAC start requested: top=%u sample_rate=%uHz waveform=%uHz", tim6_top_ticks_, sample_rate_hz_,
            profile_.frequency.value);
    return ErrorCode::Ok;
}

Result<void> DacControlManager::stop() {
    if (!is_running()) {
        return ErrorCode::Ok;
    }

    (void)counter_stop(tim6_counter_dev_);
    stop_dma();
    atomic_set(&running_, 0);
    LOG_INF("DAC stop requested");
    return ErrorCode::Ok;
}

Result<void> DacControlManager::submit_block(const uint16_t* samples, size_t count) {
    if (samples == nullptr || count == 0U) {
        return ErrorCode::InvalidArgument;
    }

    /* Placeholder for future DMA enqueue implementation. */
    return ErrorCode::Ok;
}

bool DacControlManager::is_ready() const { return ready_; }

bool DacControlManager::is_running() const { return atomic_get(&running_) != 0; }

uint32_t DacControlManager::run_count() const { return run_count_; }

void DacControlManager::init() {
    dac_dev_          = DEVICE_DT_GET(DT_NODELABEL(dac1));
    tim6_counter_dev_ = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(timers6), counter));
    dma_dev_          = DEVICE_DT_GET(DT_DMAS_CTLR_BY_NAME(DT_NODELABEL(dac1), tx));
    dma_channel_      = DT_DMAS_CELL_BY_NAME(DT_NODELABEL(dac1), tx, channel);

    if (!device_is_ready(dac_dev_)) {
        LOG_ERR("DAC device not ready");
        ready_ = false;
        return;
    }

    if (!device_is_ready(tim6_counter_dev_)) {
        LOG_ERR("TIM6 counter device not ready");
        ready_ = false;
        return;
    }

    if (!device_is_ready(dma_dev_)) {
        LOG_ERR("DAC DMA device not ready");
        ready_ = false;
        return;
    }

    dac_cfg_.channel_id = kDacChannelId;
    dac_cfg_.resolution = kDacResolutionBit;
    dac_cfg_.buffered   = true;
    dac_cfg_.internal   = false;

    int rc = dac_channel_setup(dac_dev_, &dac_cfg_);
    if (rc != 0) {
        LOG_ERR("DAC channel setup failed: %d", rc);
        ready_ = false;
        return;
    }

    timer_input_hz_ = counter_get_frequency(tim6_counter_dev_);
    if (timer_input_hz_ == 0U) {
        LOG_ERR("TIM6 frequency read failed");
        ready_ = false;
        return;
    }

    tim6_top_cfg_.callback  = nullptr;
    tim6_top_cfg_.user_data = nullptr;
    tim6_top_cfg_.flags     = 0U;

    Result<void> profile_rc = apply_runtime_profile();
    if (profile_rc.is_error()) {
        LOG_ERR("Runtime profile apply failed: %d", static_cast<int>(profile_rc.error()));
        ready_ = false;
        return;
    }

    rc = counter_set_top_value(tim6_counter_dev_, &tim6_top_cfg_);
    if (rc != 0) {
        LOG_ERR("TIM6 top setup failed: %d", rc);
        ready_ = false;
        return;
    }

    configure_tim6_trgo_update();

    const uint32_t dma_cfg_word = DT_DMAS_CELL_BY_NAME(DT_NODELABEL(dac1), tx, channel_config);
    dma_cfg_                    = {};
    dma_cfg_.dma_slot           = DT_DMAS_CELL_BY_NAME(DT_NODELABEL(dac1), tx, slot);
    dma_cfg_.channel_direction  = STM32_DMA_CONFIG_DIRECTION(dma_cfg_word);
    dma_cfg_.source_data_size   = STM32_DMA_CONFIG_MEMORY_DATA_SIZE(dma_cfg_word);
    dma_cfg_.dest_data_size     = STM32_DMA_CONFIG_PERIPHERAL_DATA_SIZE(dma_cfg_word);
    /* STM32 DMA burst length accepts 1/4/8/16. Use single burst for DAC feed. */
    dma_cfg_.source_burst_length = 1U;
    dma_cfg_.dest_burst_length   = 1U;
    dma_cfg_.channel_priority    = STM32_DMA_CONFIG_PRIORITY(dma_cfg_word);
    dma_cfg_.dma_callback        = &DacControlManager::on_dma_complete;
    dma_cfg_.user_data           = this;
    dma_cfg_.block_count         = 1U;

    dma_block_cfg_ = {};
    dma_block_cfg_.source_address = reinterpret_cast<uint32_t>(&wave_buffer_[0]);
    dma_block_cfg_.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
    dma_block_cfg_.source_reload_en = 1U;
    dma_block_cfg_.dest_address =
        LL_DAC_DMA_GetRegAddr(reinterpret_cast<DAC_TypeDef*>(DT_REG_ADDR(DT_NODELABEL(dac1))), LL_DAC_CHANNEL_1,
                              LL_DAC_DMA_REG_DATA_12BITS_RIGHT_ALIGNED);
    dma_block_cfg_.dest_addr_adj      = DMA_ADDR_ADJ_NO_CHANGE;
    dma_block_cfg_.dest_reload_en     = 1U;
    dma_block_cfg_.block_size         = kWaveTableSize * sizeof(uint16_t);
    dma_block_cfg_.fifo_mode_control  = 0U;
    dma_cfg_.head_block               = &dma_block_cfg_;
    dma_cfg_.cyclic                   = 1U;
    dma_cfg_.complete_callback_en     = 1U;
    dma_cfg_.half_complete_callback_en = 0U;
    dma_cfg_.error_callback_dis       = 0U;

    (void)dac_write_value(dac_dev_, kDacChannelId, wave_buffer_[0]);

    ready_ = true;
    LOG_INF("DAC manager initialized: dma_ch=%u dma_slot=%u timer_in=%uHz top=%u sample_rate=%uHz", dma_channel_,
            dma_cfg_.dma_slot, timer_input_hz_, tim6_top_ticks_, sample_rate_hz_);
}

void DacControlManager::run() {
    run_count_++;

    if (!is_running()) {
        return;
    }
}

void DacControlManager::on_dma_complete(const struct device* dev, void* user_data, uint32_t channel, int status) {
    ARG_UNUSED(dev);
    ARG_UNUSED(channel);

    auto* self = static_cast<DacControlManager*>(user_data);
    if (self == nullptr) {
        return;
    }

    if (status < 0) {
        self->dma_error_count_++;
        LOG_ERR("DAC DMA error: status=%d count=%u", status, self->dma_error_count_);
    }
}

Result<void> DacControlManager::apply_runtime_profile() {
    uint32_t waveform_hz = profile_.frequency.value == 0U ? kDefaultFreqHz : profile_.frequency.value;
    uint32_t requested_sample_rate = profile_.sample_rate.value;

    if (timer_input_hz_ == 0U) {
        return ErrorCode::InvalidState;
    }

    uint32_t target_sample_rate = requested_sample_rate;
    if (target_sample_rate == 0U) {
        target_sample_rate = waveform_hz * kWaveTableSize;
    }
    if (target_sample_rate == 0U || target_sample_rate > timer_input_hz_) {
        return ErrorCode::InvalidArgument;
    }

    uint32_t top_ticks = (timer_input_hz_ / target_sample_rate);
    if (top_ticks == 0U) {
        return ErrorCode::InvalidArgument;
    }

    top_ticks -= 1U;
    if (top_ticks < 1U) {
        return ErrorCode::InvalidArgument;
    }

    sample_rate_hz_     = timer_input_hz_ / (top_ticks + 1U);
    tim6_top_ticks_     = top_ticks;
    tim6_top_cfg_.ticks = top_ticks;

    if (ready_ && tim6_counter_dev_ != nullptr) {
        int rc = counter_set_top_value(tim6_counter_dev_, &tim6_top_cfg_);
        if (rc != 0) {
            LOG_ERR("TIM6 runtime top update failed: %d", rc);
            return ErrorCode::HardwareFault;
        }
    }

    Result<void> wave_rc = rebuild_wave_buffer();
    if (wave_rc.is_error()) {
        return wave_rc.error();
    }

    flush_wave_buffer_cache(wave_buffer_, sizeof(wave_buffer_));

    LOG_INF("Wave[0..7]=%u,%u,%u,%u,%u,%u,%u,%u", wave_buffer_[0], wave_buffer_[1], wave_buffer_[2], wave_buffer_[3],
            wave_buffer_[4], wave_buffer_[5], wave_buffer_[6], wave_buffer_[7]);

    return ErrorCode::Ok;
}

uint16_t DacControlManager::calc_sine_sample(uint32_t phase_index) const {
    const float phase = (kTwoPi * static_cast<float>(phase_index % kWaveTableSize)) /
                        static_cast<float>(kWaveTableSize);
    const int32_t norm_milli = static_cast<int32_t>(sinf(phase) * 1000.0F);
    return convert_mv_to_dac_code(calc_level_mv_from_norm(norm_milli));
}

uint16_t DacControlManager::calc_triangle_sample(uint32_t phase_index) const {
    const uint32_t idx = phase_index % kWaveTableSize;
    const uint32_t half = kWaveTableSize / 2U;
    int32_t norm_milli = 0;

    if (idx < half) {
        norm_milli = -1000 + static_cast<int32_t>((2000U * idx) / half);
    } else {
        norm_milli = 1000 - static_cast<int32_t>((2000U * (idx - half)) / half);
    }

    return convert_mv_to_dac_code(calc_level_mv_from_norm(norm_milli));
}

uint16_t DacControlManager::calc_saw_sample(uint32_t phase_index) const {
    const uint32_t idx = phase_index % kWaveTableSize;
    const int32_t norm_milli = -1000 + static_cast<int32_t>((2000U * idx) / kWaveTableSize);
    return convert_mv_to_dac_code(calc_level_mv_from_norm(norm_milli));
}

uint16_t DacControlManager::calc_square_sample(uint32_t phase_index) const {
    const uint32_t idx = phase_index % kWaveTableSize;
    const uint32_t duty_permille = profile_.duty.value > 1000U ? 1000U : profile_.duty.value;
    const uint32_t high_count = (kWaveTableSize * duty_permille) / 1000U;
    const bool high = idx < high_count;
    const int32_t norm_milli = high ? 1000 : -1000;
    return convert_mv_to_dac_code(calc_level_mv_from_norm(norm_milli));
}

uint16_t DacControlManager::convert_mv_to_dac_code(int32_t mv) const {
    int32_t clamped = mv;
    if (clamped < 0) {
        clamped = 0;
    } else if (clamped > static_cast<int32_t>(kDacVrefMv)) {
        clamped = static_cast<int32_t>(kDacVrefMv);
    }

    const uint32_t code =
        (static_cast<uint32_t>(clamped) * kDacFullScale + (kDacVrefMv / 2U)) / kDacVrefMv;
    return static_cast<uint16_t>(code & kDacFullScale);
}

int32_t DacControlManager::calc_level_mv_from_norm(int32_t norm_milli) const {
    const int32_t half_amplitude = profile_.amplitude.value / 2;
    return profile_.offset.value + (half_amplitude * norm_milli) / 1000;
}

Result<void> DacControlManager::rebuild_wave_buffer() {
    if (profile_.duty.value > 1000U) {
        return ErrorCode::InvalidArgument;
    }

    for (uint32_t i = 0U; i < kWaveTableSize; ++i) {
        switch (profile_.kind) {
            case WaveformKind::Sine:
                wave_buffer_[i] = calc_sine_sample(i);
                break;
            case WaveformKind::Triangle:
                wave_buffer_[i] = calc_triangle_sample(i);
                break;
            case WaveformKind::Sawtooth:
                wave_buffer_[i] = calc_saw_sample(i);
                break;
            case WaveformKind::Square:
            case WaveformKind::Pwm:
                wave_buffer_[i] = calc_square_sample(i);
                break;
            case WaveformKind::Arbitrary:
            case WaveformKind::None:
            default:
                wave_buffer_[i] = convert_mv_to_dac_code(profile_.offset.value);
                break;
        }
    }

    return ErrorCode::Ok;
}

Result<void> DacControlManager::start_dma() {
    configure_tim6_trgo_update();
    flush_wave_buffer_cache(wave_buffer_, sizeof(wave_buffer_));

    int rc = dma_config(dma_dev_, dma_channel_, &dma_cfg_);
    if (rc != 0) {
        LOG_ERR("DAC DMA config failed: %d", rc);
        return ErrorCode::HardwareFault;
    }

    rc = dma_start(dma_dev_, dma_channel_);
    if (rc != 0) {
        LOG_ERR("DAC DMA start failed: %d", rc);
        return ErrorCode::HardwareFault;
    }

    LL_DAC_SetTriggerSource(reinterpret_cast<DAC_TypeDef*>(DT_REG_ADDR(DT_NODELABEL(dac1))), LL_DAC_CHANNEL_1,
                            LL_DAC_TRIG_EXT_TIM6_TRGO);
    LL_DAC_EnableTrigger(reinterpret_cast<DAC_TypeDef*>(DT_REG_ADDR(DT_NODELABEL(dac1))), LL_DAC_CHANNEL_1);
    LL_DAC_EnableDMAReq(reinterpret_cast<DAC_TypeDef*>(DT_REG_ADDR(DT_NODELABEL(dac1))), LL_DAC_CHANNEL_1);

    LOG_INF("DAC DMA started: ch=%u slot=%u trgo=UPDATE", dma_channel_, dma_cfg_.dma_slot);
    dma_configured_ = true;
    return ErrorCode::Ok;
}

void DacControlManager::stop_dma() {
    if (!dma_configured_) {
        return;
    }

    LL_DAC_DisableDMAReq(reinterpret_cast<DAC_TypeDef*>(DT_REG_ADDR(DT_NODELABEL(dac1))), LL_DAC_CHANNEL_1);
    (void)dma_stop(dma_dev_, dma_channel_);
    dma_configured_ = false;
}

} // namespace omnigen
