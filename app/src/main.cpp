#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_saadc.h>


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

#define ADC_CONTROL_NODE DT_ALIAS(adc_control)
#if !DT_NODE_HAS_STATUS(ADC_CONTROL_NODE, okay)
#error "Unsupported board: ADC_CONTROL_NODE device tree alias is not defined"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED4_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON0_NODE, gpios);
static const struct gpio_dt_spec vext_ctl = GPIO_DT_SPEC_GET(VEXT_CONTROL_NODE, gpios);
static const struct gpio_dt_spec adc_ctl = GPIO_DT_SPEC_GET(ADC_CONTROL_NODE, gpios);

static const struct device *strip;
static const struct device *adc_dev;

static int16_t sample_buffer[1];
static const struct adc_sequence sequence = {
    .channels = BIT(2),  // Channel 2 (AIN2)
    .buffer = sample_buffer,
    .buffer_size = sizeof(sample_buffer),
    .resolution = 12,
};

static const struct adc_channel_cfg channel_cfg = {
    .gain = ADC_GAIN_1_6,                    
    .reference = ADC_REF_INTERNAL,           
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id = 2,                         
    .differential = 0,                       
    .input_positive = NRF_SAADC_INPUT_AIN2,  
    .input_negative = NRF_SAADC_INPUT_DISABLED,
};


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
    COLOR_OFF,
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

int vext_config() {
    int ret = 0;
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

    LOG_INF("VEXT control pin configured and activated correctly");
    return 0;
}

int led_config() {
    int ret = 0;
    if (!device_is_ready(led.port)) {
        LOG_ERR("LED not ready");
        return 1;
    }
    
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
    if (ret < 0) {
        LOG_ERR("LED configure failed: %d", ret);
        return 1;
    }

    LOG_INF("Green led configured correctly");
    return 0;
}

int button_config() {
    int ret = 0;
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
    
    LOG_INF("Button configured correctly!");
    return 0;
}

int battery_read_config(){
    int ret  = 0;
    if (!device_is_ready(adc_ctl.port)) {
        LOG_ERR("ADC control GPIO not ready");
        return 1;
    }
    
    ret = gpio_pin_configure_dt(&adc_ctl, GPIO_OUTPUT);
    if (ret < 0) {
        LOG_ERR("ADC control configure failed: %d", ret);
        return 1;
    }
    
    ret = gpio_pin_set_dt(&adc_ctl, 1);
    if (ret < 0) {
        LOG_ERR("ADC control set failed: %d", ret);
        return 1;
    }
    k_sleep(K_MSEC(10)); 
    
    adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));
    if (!device_is_ready(adc_dev)) {
        LOG_ERR("ADC device not ready");
        return 1;
    }

    ret = adc_channel_setup(adc_dev, &channel_cfg);
    if (ret < 0) {
        LOG_ERR("ADC channel setup failed: %d", ret);
        return 1;
    }

    LOG_INF("ADC initialized successfully");
    return 0;
}

int main(void) {
    int ret;

    LOG_INF("Hello, World!\n");

    ret = vext_config();
    if(ret) return ret;

    ret = led_config();
    if(ret) return ret;

    ret = button_config();
    if(ret) return ret;

    ret = sk6812_init();
    if(ret) return ret;

    ret = battery_read_config();
    if(ret) return ret;

    while (1) {
        ret = adc_read(adc_dev, &sequence);
        if (ret == 0) {
            LOG_INF("ADC AIN2 Value: %d", sample_buffer[0]);
        } else {
            LOG_ERR("ADC read failed: %d", ret);
        }
        k_sleep(K_MSEC(2000));
    }

    return 0;
}