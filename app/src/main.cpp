#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_saadc.h>

#include <zephyr/drivers/display.h>
#include <zephyr/sys/byteorder.h>

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

#define TFT_EN_NODE DT_ALIAS(tft_en)
#if !DT_NODE_HAS_STATUS(TFT_EN_NODE, okay)
#error "Unsupported board: TFT_EN_NODE device tree alias is not defined"
#endif

#define TFT_LED_EN DT_ALIAS(tft_led_en)
#if !DT_NODE_HAS_STATUS(TFT_LED_EN, okay)
#error "Unsupported board: TFT_LED_EN device tree alias is not defined"
#endif


static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED4_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON0_NODE, gpios);
static const struct gpio_dt_spec vext_ctl = GPIO_DT_SPEC_GET(VEXT_CONTROL_NODE, gpios);
static const struct gpio_dt_spec adc_ctl = GPIO_DT_SPEC_GET(ADC_CONTROL_NODE, gpios);
static const struct gpio_dt_spec tft_en = GPIO_DT_SPEC_GET(TFT_EN_NODE, gpios);
static const struct gpio_dt_spec tft_led_en = GPIO_DT_SPEC_GET(TFT_LED_EN, gpios);

static const struct device *strip;
static const struct device *adc_dev;
static const struct device *display;


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

int tft_init() {
    display = DEVICE_DT_GET(DT_NODELABEL(tft_display));
    if (!device_is_ready(display)) {
        LOG_ERR("TFT display not ready");
        return -1;
    }
    
    LOG_INF("TFT display initialized");
    return 0;
}

int tft_enable_display() {
    
    int ret  = 0;
    if (!device_is_ready(tft_en.port)) {
        LOG_ERR("TFT_EN GPIO not ready");
        return 1;
    }
    
    ret = gpio_pin_configure_dt(&tft_en, GPIO_OUTPUT);
    if (ret < 0) {
        LOG_ERR("TFT_EN configure failed: %d", ret);
        return 1;
    }
    
    ret = gpio_pin_set_dt(&tft_en, 1);
    if (ret < 0) {
        LOG_ERR("TFT_EN set failed: %d", ret);
        return 1;
    }

    k_msleep(5);
    LOG_INF("TFT_EN pin configured correctly!");
    return 0;
}
// SYS_INIT(tft_enable_display, PRE_KERNEL_2, 0);

int tft_enable_backlight() {
    int ret  = 0;

    if (!device_is_ready(tft_led_en.port)) {
        LOG_ERR("TFT_LED_EN GPIO not ready");
        return 1;
    }
    
    ret = gpio_pin_configure_dt(&tft_led_en, GPIO_OUTPUT);
    if (ret < 0) {
        LOG_ERR("TFT_LED_EN configure failed: %d", ret);
        return 1;
    }
    
    ret = gpio_pin_set_dt(&tft_led_en, GPIO_ACTIVE_LOW);
    if (ret < 0) {
        LOG_ERR("TFT_LED_EN set failed: %d", ret);
        return 1;
    }
    k_msleep(5);
    LOG_INF("TFT_LED_EN pin configured correctly!");
    return 0;
}


void tft_clear(uint16_t color) {
    struct display_buffer_descriptor desc;
    static uint16_t line_buf[135];  // One line of pixels
    
    // Fill buffer with color
    for (int i = 0; i < 135; i++) {
        line_buf[i] = color;
    }
    
    desc.width = 135;
    desc.height = 1;
    desc.pitch = 135;
    desc.buf_size = sizeof(line_buf);
    
    // Write line by line
    for (int y = 0; y < 240; y++) {
        display_write(display, 0, y, &desc, line_buf);
    }
}

void tft_draw_border(uint16_t color) {
    uint16_t c = sys_cpu_to_be16(color);
    struct display_buffer_descriptor desc;
    
    static uint16_t h_line[135];
    for (int i = 0; i < 135; i++) h_line[i] = c;
    
    desc.width = 135;
    desc.height = 1;
    desc.pitch = 135;
    desc.buf_size = sizeof(h_line);
    
    display_write(display, 0, 0, &desc, h_line);      // Top edge
    display_write(display, 0, 239, &desc, h_line);    // Bottom edge
    
    // Left and right vertical lines (1 pixel wide, full height)
    static uint16_t pixel[1];
    pixel[0] = c;
    
    desc.width = 1;
    desc.height = 1;
    desc.pitch = 1;
    desc.buf_size = sizeof(pixel);
    
    for (int y = 0; y < 240; y++) {
        display_write(display, 0, y, &desc, pixel);    // Left edge
        display_write(display, 134, y, &desc, pixel);  // Right edge
    }
}

void tft_draw_corners(void) {
    struct display_buffer_descriptor desc;
    static uint16_t block[10 * 10];  // 10x10 pixel blocks
    
    desc.width = 10;
    desc.height = 10;
    desc.pitch = 10;
    desc.buf_size = sizeof(block);
    
    // Top-left: Red
    for (int i = 0; i < 100; i++) block[i] = sys_cpu_to_be16(0xF800);
    display_write(display, 0, 0, &desc, block);
    
    // Top-right: Green
    for (int i = 0; i < 100; i++) block[i] = sys_cpu_to_be16(0x07E0);
    display_write(display, 125, 0, &desc, block);  // 135 - 10 = 125
    
    // Bottom-left: Blue
    for (int i = 0; i < 100; i++) block[i] = sys_cpu_to_be16(0x001F);
    display_write(display, 0, 230, &desc, block);  // 240 - 10 = 230
    
    // Bottom-right: White
    for (int i = 0; i < 100; i++) block[i] = sys_cpu_to_be16(0xFFFF);
    display_write(display, 125, 230, &desc, block);
}

int tft_config() {
    if (tft_enable_display() < 0) {
        LOG_ERR("Failed to enable TFT display");
        return 1;
    }
    
    if (tft_enable_backlight() < 0) {
        LOG_ERR("Failed to enable TFT backlight");
        return 1;
    }
    
    if (tft_init() < 0) {
        LOG_ERR("Failed to initialize TFT display");
        return 1;
    }
    
    LOG_INF("Display blanking off");
    display_blanking_off(display);
    k_msleep(1000);
    
    LOG_INF("Display fill");
    
    struct display_buffer_descriptor buf_desc = {
        .width = 1,           // Width in pixels
        .height = 1,          // Height in pixels  
        .pitch = 2,           // Bytes per line (2 for RGB565)
    };
    
    // Test pixel data (RGB565 red)
    uint8_t test_pixel[2] = {0xF8, 0x00};
    
    // Write single pixel
    int ret = display_write(display, 10, 10, &buf_desc, test_pixel);
    if (ret < 0) {
        LOG_ERR("Display write failed: %d", ret);
    }
    k_msleep(1000);
    
    LOG_INF("Display black");
    tft_clear(sys_cpu_to_be16(0x0000));
    k_msleep(2000);

    LOG_INF("Display red");
    tft_clear(sys_cpu_to_be16(0xF800));  // Red
    k_msleep(2000);
    
    LOG_INF("Display green");
    tft_clear(sys_cpu_to_be16(0x07E0));  // Green
    k_msleep(2000);
    
    LOG_INF("Display blue");
    tft_clear(sys_cpu_to_be16(0x001F));  // Blue
    k_msleep(2000);
    
    LOG_INF("Display white");
    tft_clear(sys_cpu_to_be16(0xFFFF));  // White

    LOG_INF("Display black");
    tft_clear(sys_cpu_to_be16(0x0000));
    k_msleep(500);

    LOG_INF("Display white border");
    tft_draw_border(0xFFFF);
    k_msleep(2000);

    LOG_INF("Display corner markers");
    tft_draw_corners();
    k_msleep(2000);




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
    
    ret = tft_config();
    if(ret) return ret;

    k_msleep(1000);

    LOG_INF("============ Starting main loop ============");
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