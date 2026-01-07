#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/uart.h>  // Add this for UART/USB serial
#include <zephyr/logging/log.h>  // Replace stdio.h with logging header

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define LED4_NODE DT_ALIAS(led4)
#if !DT_NODE_HAS_STATUS(LED4_NODE, okay)
#error "Unsupported board: led4 device tree alias is not defined"
#endif

#define BUTTON0_NODE DT_ALIAS(button0)
#if !DT_NODE_HAS_STATUS(BUTTON0_NODE, okay)
#error "Unsupported board: button0 device tree alias is not defined"
#endif

#define STRIP_NODE DT_NODELABEL(led_strip)
#if !DT_NODE_HAS_STATUS(STRIP_NODE, okay)
#error "Unsupported board: led_strip device tree node is not defined"
#endif

#define VEXT_CONTROL_NODE DT_ALIAS(vext_control)
#if !DT_NODE_HAS_STATUS(VEXT_CONTROL_NODE, okay)
#error "Unsupported board: VEXT_NODE device tree alias is not defined"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED4_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON0_NODE, gpios);
static const struct device *strip;
static const struct gpio_dt_spec vext_ctl = GPIO_DT_SPEC_GET(VEXT_CONTROL_NODE, gpios);

static struct gpio_callback button_cb_data;


int sk6812_init(void)
{
    strip = DEVICE_DT_GET(STRIP_NODE);
    if (!device_is_ready(strip)) {
        LOG_ERR("SK6812 strip not ready");
        return 1;
    }
    return 0;
}


#define COLOR_OFF     { .r = 0x00, .g = 0x00, .b = 0x00 }
#define COLOR_RED     { .r = 0xFF, .g = 0x00, .b = 0x00 }
#define COLOR_GREEN   { .r = 0x00, .g = 0xFF, .b = 0x00 }
#define COLOR_BLUE    { .r = 0x00, .g = 0x00, .b = 0xFF }
#define COLOR_WHITE   { .r = 0xFF, .g = 0xFF, .b = 0xFF }


led_rgb colors[] = {
    COLOR_OFF,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_WHITE,
};

uint8_t pixel0_index = 0;
uint8_t pixel1_index = 1;


int sk6812_pixel_rotate() {
    struct led_rgb pixels[2] = {
        colors[pixel0_index],
        colors[pixel1_index]};

    pixel0_index = (pixel0_index + 1) % ARRAY_SIZE(colors);
    pixel1_index = (pixel1_index + 1) % ARRAY_SIZE(colors);

    int ret = led_strip_update_rgb(strip, pixels, 2);
    if (ret) {
        LOG_ERR("LED update failed: %d", ret);
        return 1;
    }

    return 0;
}

int vext_activate() {
    return gpio_pin_set_dt(&vext_ctl, 1);
}

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    static uint8_t counter = 1;
    gpio_pin_toggle_dt(&led);
    sk6812_pixel_rotate();
    LOG_INF("Button pressed %d times", counter++);
}

int main(void) {
    int ret;

    LOG_INF("Hello, World!\n");

    
    if (!device_is_ready(vext_ctl.port)) {
        LOG_ERR("VEXT not ready");
        return 1;
    }

    if (gpio_pin_configure_dt(&vext_ctl, GPIO_OUTPUT) < 0) {
        LOG_ERR("VEXT configure failed: %d", ret);
        return 1;
    }

    if(vext_activate()) {
        LOG_ERR("Couldn't activate VEXT");
        return 1;
    }

    if (!device_is_ready(led.port)) {
        LOG_ERR("LED not ready");
        return 1;
    }
    
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
    if (ret < 0) {
        LOG_ERR("LED configure failed: %d", ret);
        return 1;
    }

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

    ret = sk6812_init();
    if (ret < 0) {
        LOG_ERR("Couldn't initialize led strip");
        return 1;
    }

    while (1) {
        k_sleep(K_MSEC(500));  // Check twice per second
    }

    return 0;
}