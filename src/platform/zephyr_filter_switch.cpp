/**
 *******************************************************************************
 * @file    zephyr_filter_switch.cpp
 * @brief   Zephyr GPIO implementation of FilterSwitchPort
 *******************************************************************************
 * @attention
 *
 * This file controls the analog filter bank GPIOs through devicetree aliases.
 *
 *******************************************************************************
 * @note
 *
 * The default mounted state is bypass with the filter switch enabled and output
 * unmuted.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "platform/zephyr_filter_switch.hpp"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zephyr_filter_switch, CONFIG_LOG_DEFAULT_LEVEL);

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

#define FILTER_SEL0_NODE  DT_ALIAS(filter_sel0)
#define FILTER_SEL1_NODE  DT_ALIAS(filter_sel1)
#define FILTER_EN_NODE    DT_ALIAS(filter_en)
#define OUTPUT_MUTE_NODE  DT_ALIAS(output_mute)

static const struct gpio_dt_spec g_filter_sel0 = GPIO_DT_SPEC_GET(FILTER_SEL0_NODE, gpios);
static const struct gpio_dt_spec g_filter_sel1 = GPIO_DT_SPEC_GET(FILTER_SEL1_NODE, gpios);
static const struct gpio_dt_spec g_filter_en = GPIO_DT_SPEC_GET(FILTER_EN_NODE, gpios);
static const struct gpio_dt_spec g_output_mute = GPIO_DT_SPEC_GET(OUTPUT_MUTE_NODE, gpios);

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
        default:
            return ErrorCode::IoError;
    }
}

static bool gpio_ready()
{
    return gpio_is_ready_dt(&g_filter_sel0) && gpio_is_ready_dt(&g_filter_sel1) && gpio_is_ready_dt(&g_filter_en)
        && gpio_is_ready_dt(&g_output_mute);
}

static Result<void> set_gpio(const struct gpio_dt_spec& gpio, int value)
{
    return map_errno_to_result(gpio_pin_set_dt(&gpio, value));
}

static void filter_bits(FilterMode mode, int& sel0, int& sel1)
{
    switch (mode) {
        case FilterMode::Bypass:
            sel0 = 0;
            sel1 = 0;
            break;
        case FilterMode::Low25k:
            sel0 = 1;
            sel1 = 0;
            break;
        case FilterMode::Mid125k:
            sel0 = 0;
            sel1 = 1;
            break;
        case FilterMode::High225k:
            sel0 = 1;
            sel1 = 1;
            break;
    }
}

/*-------- 4. implementation -----------------------------------------------------------------------------------------*/

Result<void> ZephyrFilterSwitch::mount()
{
    if (!gpio_ready()) {
        LOG_ERR("Filter switch GPIOs not ready");
        return ErrorCode::InvalidState;
    }

    int ret = gpio_pin_configure_dt(&g_filter_sel0, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        return map_errno_to_result(ret);
    }

    ret = gpio_pin_configure_dt(&g_filter_sel1, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        return map_errno_to_result(ret);
    }

    ret = gpio_pin_configure_dt(&g_filter_en, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) {
        return map_errno_to_result(ret);
    }

    ret = gpio_pin_configure_dt(&g_output_mute, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        return map_errno_to_result(ret);
    }

    mounted_ = true;
    muted_ = false;
    mode_ = FilterMode::Bypass;
    LOG_INF("Filter switch mounted in bypass mode");
    return ErrorCode::Ok;
}

Result<void> ZephyrFilterSwitch::set_mode(FilterMode mode)
{
    if (!mounted_) {
        return ErrorCode::InvalidState;
    }

    int sel0 = 0;
    int sel1 = 0;
    filter_bits(mode, sel0, sel1);

    auto ret = set_mute(true);
    if (ret.is_error()) {
        return ret;
    }

    ret = set_gpio(g_filter_en, 0);
    if (ret.is_error()) {
        return ret;
    }

    ret = set_gpio(g_filter_sel0, sel0);
    if (ret.is_error()) {
        return ret;
    }

    ret = set_gpio(g_filter_sel1, sel1);
    if (ret.is_error()) {
        return ret;
    }

    k_msleep(1);

    ret = set_gpio(g_filter_en, 1);
    if (ret.is_error()) {
        return ret;
    }

    k_msleep(5);

    ret = set_mute(false);
    if (ret.is_error()) {
        return ret;
    }

    mode_ = mode;
    return ErrorCode::Ok;
}

Result<void> ZephyrFilterSwitch::set_mute(bool enabled)
{
    if (!mounted_) {
        return ErrorCode::InvalidState;
    }

    auto ret = set_gpio(g_output_mute, enabled ? 1 : 0);
    if (ret.is_ok()) {
        muted_ = enabled;
    }

    return ret;
}

} // namespace omnigen
