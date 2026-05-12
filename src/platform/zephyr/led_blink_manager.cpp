/**
 *******************************************************************************
 * @file    led_blink_manager.cpp
 * @brief   Zephyr LED blink manager thread implementation
 *******************************************************************************
 */

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "platform/zephyr/led_blink_manager.hpp"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led_blink_manager, CONFIG_LOG_DEFAULT_LEVEL);

namespace omnigen {

/* ------- variables -------------------------------------------------------------------------------------------------*/

static_assert(DT_NODE_HAS_STATUS(DT_ALIAS(led0), okay), "Unsupported board: led0 alias is not defined");

#define APPLICATION_ENABLE     true
#define APPLICATION_NAME       "led_blink"
#define APPLICATION_STACK_SIZE 1024
#define APPLICATION_PRIORITY   7

K_THREAD_STACK_DEFINE(g_led_blink_stack, APPLICATION_STACK_SIZE);

[[maybe_unused]] static auto& forceInit = LedBlinkManager::instance();

/* ------- function implement ----------------------------------------------------------------------------------------*/

LedBlinkManager::LedBlinkManager()
    : PeriodicApp(APPLICATION_ENABLE, APPLICATION_NAME, K_THREAD_STACK_SIZEOF(g_led_blink_stack), g_led_blink_stack,
                  APPLICATION_PRIORITY, kBlinkHalfPeriodMs),
      led_(GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios)) {}

void LedBlinkManager::set_blink_enabled(bool enabled) {
    atomic_set(&blink_enabled_, enabled ? 1 : 0);

    if (!enabled && ready_) {
        led_state_ = false;
        (void)gpio_pin_set_dt(&led_, 0);
    }
}

bool LedBlinkManager::is_blink_enabled() const { return atomic_get(&blink_enabled_) != 0; }

bool LedBlinkManager::is_ready() const { return ready_; }

void LedBlinkManager::init() {
    if (!gpio_is_ready_dt(&led_)) {
        LOG_ERR("LED GPIO device is not ready");
        ready_ = false;
        return;
    }

    const int rc = gpio_pin_configure_dt(&led_, GPIO_OUTPUT_INACTIVE);
    if (rc != 0) {
        LOG_ERR("Failed to configure LED GPIO: %d", rc);
        ready_ = false;
        return;
    }

    ready_ = true;
    LOG_INF("LED blink manager initialized: port=%p pin=%d flags=0x%x", static_cast<const void*>(led_.port), led_.pin,
            static_cast<unsigned int>(led_.dt_flags));
}

void LedBlinkManager::run() {
    if (!ready_) {
        LOG_WRN_ONCE("LED manager is not ready, skip run loop");
        return;
    }

    if (!is_blink_enabled()) {
        LOG_WRN_ONCE("LED blinking is disabled");
        return;
    }

    led_state_ = !led_state_;
    toggle_count_++;
    const int rc = gpio_pin_set_dt(&led_, led_state_ ? 1 : 0);
    if (rc != 0) {
        LOG_ERR("LED toggle failed: rc=%d count=%u state=%u", rc, toggle_count_, led_state_ ? 1U : 0U);
        return;
    }

}

} // namespace omnigen
