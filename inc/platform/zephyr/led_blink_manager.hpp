/**
 *******************************************************************************
 * @file    led_blink_manager.hpp
 * @brief   Zephyr LED blink manager thread
 *******************************************************************************
 */

#ifndef OMNIGEN_H7_APP_PLATFORM_ZEPHYR_LED_BLINK_MANAGER_HPP
#define OMNIGEN_H7_APP_PLATFORM_ZEPHYR_LED_BLINK_MANAGER_HPP

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "base/singleton.hpp"
#include "platform/zephyr/application_base.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/atomic.h>

namespace omnigen {

/* ------- class -----------------------------------------------------------------------------------------------------*/

class LedBlinkManager final : public PeriodicApp, public Singleton<LedBlinkManager> {
    friend class Singleton<LedBlinkManager>;

  public:
    LedBlinkManager(const LedBlinkManager&)            = delete;
    LedBlinkManager& operator=(const LedBlinkManager&) = delete;

    void set_blink_enabled(bool enabled);

    [[nodiscard]] bool is_blink_enabled() const;
    [[nodiscard]] bool is_ready() const;

  protected:
    void init() override;
    void run() override;

  private:
    static constexpr uint32_t kBlinkHalfPeriodMs = 500U;

    LedBlinkManager();

    struct gpio_dt_spec led_;
    atomic_t blink_enabled_{1};
    bool ready_{false};
    bool led_state_{false};
    uint32_t toggle_count_{0U};
};

} // namespace omnigen

#endif /* OMNIGEN_H7_APP_PLATFORM_ZEPHYR_LED_BLINK_MANAGER_HPP */
