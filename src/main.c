#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define SLEEP_TIME_MS 1000
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void) {
  if (!gpio_is_ready_dt(&led)) {
    LOG_ERR("LED device not ready");
    return 0;
  }

  int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    LOG_ERR("Failed to configure LED pin (err %d)", ret);
    return 0;
  }

  LOG_INF("OmniGen H7 LED blinky started");

  while(1) {
    gpio_pin_toggle_dt(&led);
    k_msleep(SLEEP_TIME_MS);
  }

  return 0;
}

