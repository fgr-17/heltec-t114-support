#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/uart.h>  // Add this for UART/USB serial
#include <zephyr/logging/log.h>  // Replace stdio.h with logging header

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define LED0_NODE DT_ALIAS(led0)
#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Unsupported board: led0 device tree alias is not defined"
#endif

#define BUTTON0_NODE DT_ALIAS(button0)
#if !DT_NODE_HAS_STATUS(BUTTON0_NODE, okay)
#error "Unsupported board: button0 device tree alias is not defined"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON0_NODE, gpios);

static struct gpio_callback button_cb_data;

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_toggle_dt(&led);
    LOG_INF("Button pressed");
}

int main(void)
{
    int ret;

    LOG_INF("Hello, World!\n");

    if (!device_is_ready(led.port)) {
        LOG_ERR("LED not ready");
        return 1;
    }
    
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
    if (ret < 0) {
        LOG_ERR("LED configure failed: %d", ret);
        return 1;
    }

    // Initialize button
    if (!device_is_ready(button.port)) {
        LOG_ERR("Button not ready");
        return 1;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Button configure failed: %d", ret);
        return 1;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Button interrupt configure failed: %d", ret);
        return 1;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    // Just sit here and wait for button presses
    while (1) {
        k_sleep(K_MSEC(1000));
    }

    return 0;
}