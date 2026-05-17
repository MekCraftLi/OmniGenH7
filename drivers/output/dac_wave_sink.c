/**
 *******************************************************************************
 * @file    dac_wave_sink.c
 * @brief   DAC wave output driver using TIM trigger and DMA
 *******************************************************************************
 * @attention
 *
 * This driver is board-specific and drives DAC1 channel 1 on PA4 using TIM6 and
 * DMA for continuous waveform output.
 *
 *******************************************************************************
 * @note
 *
 * DAC samples are 12-bit right-aligned values. The DMA buffer must remain safe
 * for peripheral DMA access on STM32H723.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */
#define DT_DRV_COMPAT mekcraft_dac_wave_sink

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>

#include <stm32h7xx_hal.h>
#include <stm32h7xx_hal_dac.h>
#include <stm32h7xx_hal_tim.h>
#include <stm32h7xx_hal_dma.h>
#include <stm32h7xx_ll_tim.h>
#include <stm32h7xx_ll_dac.h>
#include <stm32h7xx_ll_dma.h>

LOG_MODULE_REGISTER(dac_wave_sink, CONFIG_LOG_DEFAULT_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(mekcraft_dac_wave_sink)

/*-------- 2. data structures ----------------------------------------------------------------------------------------*/

/**
 * @brief DAC 波形输出设备静态配置结构体。
 *
 * 保存从设备树和驱动实例化阶段传入的硬件资源参数，包括 DAC、定时器、DMA、
 * 缓冲区大小和默认采样率。
 */
struct dac_wave_sink_config {
    const struct device *dac_dev;
    uint32_t timer_base;
    uint32_t dma_base;
    uint32_t dma_channel;
    uint32_t sample_buffer_size;
    uint32_t sample_rate_hz;
    uint32_t dac_channel;
};

/**
 * @brief DAC 波形输出设备运行时数据结构体。
 *
 * 保存 HAL 句柄、样本缓冲区、运行状态和当前采样率。该结构由 Zephyr 设备实例
 * 持有，驱动函数通过 `dev->data` 访问。
 */
struct dac_wave_sink_data {
    DAC_HandleTypeDef hdac;
    TIM_HandleTypeDef htim;
    DMA_HandleTypeDef hdma;
    uint16_t *sample_buffer;
    uint32_t buffer_size;
    bool running;
    uint32_t sample_rate;
};

/*-------- 3. buffers ------------------------------------------------------------------------------------------------*/

#define DEFAULT_BUFFER_SIZE 256

/* Double buffer for continuous output */
static uint16_t dac_buffer[DEFAULT_BUFFER_SIZE * 2] __aligned(4);

/*-------- 4. hardware initialization --------------------------------------------------------------------------------*/

static int init_dac(const struct device *dev)
{
    const struct dac_wave_sink_config *cfg = dev->config;
    struct dac_wave_sink_data *data = dev->data;

    /* Initialize DAC HAL handle */
    data->hdac.Instance = DAC1;
    data->hdac.State = HAL_DAC_STATE_RESET;

    /* Enable DAC clock */
    __HAL_RCC_DAC12_CLK_ENABLE();

    /* Initialize DAC */
    HAL_DAC_DeInit(&data->hdac);

    DAC_ChannelConfTypeDef sConfig = {0};
    sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

    if (HAL_DAC_ConfigChannel(&data->hdac, &sConfig, cfg->dac_channel) != HAL_OK) {
        LOG_ERR("Failed to configure DAC channel");
        return -EIO;
    }

    LOG_INF("DAC initialized");
    return 0;
}

static int init_timer(const struct device *dev)
{
    struct dac_wave_sink_data *data = dev->data;

    /* Initialize TIM6 HAL handle */
    data->htim.Instance = TIM6;
    data->htim.Init.Prescaler = 0;
    data->htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    data->htim.Init.Period = 0xFFFF;  /* Will be set based on sample rate */
    data->htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    data->htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&data->htim) != HAL_OK) {
        LOG_ERR("Failed to initialize timer");
        return -EIO;
    }

    /* Configure TIM6 to generate TRGO on update event */
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(&data->htim, &sMasterConfig) != HAL_OK) {
        LOG_ERR("Failed to configure timer TRGO");
        return -EIO;
    }

    LOG_INF("Timer initialized");
    return 0;
}

static int init_dma(const struct device *dev)
{
    struct dac_wave_sink_data *data = dev->data;

    /* Initialize DMA handle */
    data->hdma.Instance = DMA1_Stream5;
    data->hdma.Init.Request = DMA_REQUEST_DAC1_CH1;
    data->hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    data->hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    data->hdma.Init.MemInc = DMA_MINC_ENABLE;
    data->hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    data->hdma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    data->hdma.Init.Mode = DMA_CIRCULAR;
    data->hdma.Init.Priority = DMA_PRIORITY_HIGH;
    data->hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&data->hdma) != HAL_OK) {
        LOG_ERR("Failed to initialize DMA");
        return -EIO;
    }

    /* Link DMA to DAC */
    __HAL_LINKDMA(&data->hdac, DMA_Handle1, data->hdma);

    /* Enable DMA interrupt */
    NVIC_SetPriority(DMA1_Stream5_IRQn, 5);
    NVIC_EnableIRQ(DMA1_Stream5_IRQn);

    LOG_INF("DMA initialized");
    return 0;
}

/*-------- 5. driver implementation ----------------------------------------------------------------------------------*/

static int dac_wave_sink_init(const struct device *dev)
{
    struct dac_wave_sink_data *data = dev->data;
    int ret;

    LOG_INF("Initializing DAC wave sink");

    /* Initialize sample buffer */
    data->sample_buffer = dac_buffer;
    data->buffer_size = DEFAULT_BUFFER_SIZE;
    data->running = false;
    data->sample_rate = 10000;  /* Default 10 kHz */

    /* Initialize hardware */
    ret = init_dac(dev);
    if (ret < 0) {
        return ret;
    }

    ret = init_timer(dev);
    if (ret < 0) {
        return ret;
    }

    ret = init_dma(dev);
    if (ret < 0) {
        return ret;
    }

    LOG_INF("DAC wave sink ready");
    return 0;
}

/*-------- 6. public api ---------------------------------------------------------------------------------------------*/

int dac_wave_sink_configure(const struct device *dev, uint32_t sample_rate)
{
    struct dac_wave_sink_data *data = dev->data;
    uint32_t timer_clock;
    uint32_t period;

    /* Get timer clock frequency */
    /* APB1 clock = 137.5 MHz on this board */
    timer_clock = 137500000;

    /* Calculate timer period for desired sample rate */
    /* Period = TimerClock / SampleRate - 1 */
    period = (timer_clock / sample_rate) - 1;

    if (period > 0xFFFF) {
        LOG_ERR("Sample rate too low, period exceeds 16-bit");
        return -EINVAL;
    }

    /* Update timer period */
    __HAL_TIM_SET_AUTORELOAD(&data->htim, period);
    data->sample_rate = sample_rate;

    LOG_INF("Sample rate set to %u Hz (period = %u)", sample_rate, period);
    return 0;
}

int dac_wave_sink_start(const struct device *dev)
{
    struct dac_wave_sink_data *data = dev->data;

    if (data->running) {
        return 0;  /* Already running */
    }

    /* Enable DAC channel */
    HAL_DAC_Start(&data->hdac, DAC_CHANNEL_1);

    /* Start DMA transfer */
    HAL_DAC_Start_DMA(&data->hdac, DAC_CHANNEL_1,
                      (uint32_t *)data->sample_buffer,
                      data->buffer_size * 2,  /* Double buffer */
                      DAC_ALIGN_12B_R);

    /* Start timer */
    HAL_TIM_Base_Start(&data->htim);

    data->running = true;
    LOG_INF("DAC output started");
    return 0;
}

int dac_wave_sink_stop(const struct device *dev)
{
    struct dac_wave_sink_data *data = dev->data;

    if (!data->running) {
        return 0;  /* Already stopped */
    }

    /* Stop timer */
    HAL_TIM_Base_Stop(&data->htim);

    /* Stop DMA */
    HAL_DAC_Stop_DMA(&data->hdac, DAC_CHANNEL_1);

    data->running = false;
    LOG_INF("DAC output stopped");
    return 0;
}

int dac_wave_sink_set_buffer(const struct device *dev, const uint16_t *samples, size_t count)
{
    struct dac_wave_sink_data *data = dev->data;

    if (count > data->buffer_size * 2) {
        LOG_ERR("Buffer too large");
        return -EINVAL;
    }

    /* Copy samples to DMA buffer */
    memcpy(data->sample_buffer, samples, count * sizeof(uint16_t));

    return 0;
}

/*-------- 7. device definition --------------------------------------------------------------------------------------*/

#define DAC_WAVE_SINK_INIT(n) \
    static struct dac_wave_sink_data dac_wave_sink_data_##n; \
    \
    static const struct dac_wave_sink_config dac_wave_sink_config_##n = { \
        .dac_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, dac)), \
        .timer_base = DT_REG_ADDR(DT_INST_PHANDLE(n, timer)), \
        .dma_base = DT_REG_ADDR(DT_INST_PHANDLE(n, dma)), \
        .sample_buffer_size = DT_INST_PROP_OR(n, sample_buffer_size, 256), \
        .sample_rate_hz = DT_INST_PROP_OR(n, sample_rate_hz, 10000), \
        .dac_channel = DAC_CHANNEL_1, \
    }; \
    \
    DEVICE_DT_INST_DEFINE(n, \
        dac_wave_sink_init, \
        NULL, \
        &dac_wave_sink_data_##n, \
        &dac_wave_sink_config_##n, \
        POST_KERNEL, \
        CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
        NULL);

DT_INST_FOREACH_STATUS_OKAY(DAC_WAVE_SINK_INIT)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(mekcraft_dac_wave_sink) */
