/**
 *******************************************************************************
 * @file    dac_wave_sink.h
 * @brief   Board-level DAC waveform output API
 *******************************************************************************
 * @attention
 *
 * This API wraps the OmniGen H7 DAC+TIM+DMA output device declared in
 * Devicetree.
 *
 *******************************************************************************
 * @note
 *
 * Calls are synchronous configuration/control operations. Sample data passed to
 * `dac_wave_sink_set_buffer` is copied into the driver-owned DMA buffer before
 * the function returns.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/19
 * @version 1.0
 *******************************************************************************
 */
#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

struct device;

int dac_wave_sink_configure(const struct device* dev, uint32_t sample_rate);
int dac_wave_sink_start(const struct device* dev);
int dac_wave_sink_stop(const struct device* dev);
int dac_wave_sink_set_buffer(const struct device* dev, const uint16_t* samples, size_t count);

#ifdef __cplusplus
}
#endif
