#include <zephyr/logging/log.h>
#include "board_init.h"
#include "led_strip_test.h"
#include "tft_display_test.h"

#include <hal/nrf_saadc.h>

#define ADC_TIME_INTERVAL  2000
#define LORA_TIME_INTERVAL 2000

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

/* LoRa configuration */
struct lora_modem_config config = {
	.frequency = 868000000,
	.bandwidth = BW_125_KHZ,
	.datarate = SF_7,
	.coding_rate = CR_4_5,
	.preamble_len = 8,
	.tx_power = 14,
	.tx = true,
};

/**
 * @brief Initialize the LoRa modem
 * @returns 0 on success, 1 on failure
 */
static int lora_init()
{
	if (lora_config(lora_dev, &config)) {
		LOG_ERR("LoRa configuration failed");
		return 1;
	}
	LOG_INF("LoRa configured successfully!");
	return 0;
}

/**
 * @brief Send a LoRa message with Waveshare DTU header
 * @param message - null-terminated string to send
 * @return 0 on success, negative error code on failure
 */

int lora_send_message_waveshare_dtu(const char *message)
{
	if (!message) {
		return -EINVAL;
	}

	uint8_t tx_buf[255];
	size_t payload_len = strlen(message);

	if (payload_len > 251) {
		payload_len = 251;
	}

	size_t total_len = 4 + payload_len;

	tx_buf[0] = 0x00; /* Address high byte */
	tx_buf[1] = 0x00; /* Address low byte */
	tx_buf[2] = 0x00; /* Network ID */
	tx_buf[3] = (uint8_t)total_len;

	memcpy(&tx_buf[4], message, payload_len);

	return lora_send(lora_dev, tx_buf, total_len);
}

/* ------------------------------------------------------------ */

/* adc configuration */
static int16_t sample_buffer[1];
static const struct adc_sequence sequence = {
	.channels = BIT(2), // Channel 2 (AIN2)
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

/**
 * @brief Configure the ADC
 * @returns 0 on success, negative error code on failure
 */

static int adc_config()
{
	if (adc_channel_setup(adc_dev, &channel_cfg)) {
		LOG_ERR("ADC channel setup failed");
		return -EINVAL;
	}
	LOG_INF("ADC configured correctly!");
	return 0;
}
/* ------------------------------------------------------------ */

/* button config*/

/**
 * @brief Callback function for button pressed
 * @param dev - device pointer
 * @param cb - callback pointer
 * @param pins - pins bitmask
 */

void button_pressed_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	static uint8_t counter = 1;
	gpio_pin_toggle_dt(&led);
	sk6812_pixel_rotate();
	LOG_INF("Button pressed %d times", counter++);
}

/**
 * @brief Configure the button
 * @returns 0 on success, negative error code on failure
 */

static int button_config()
{
	if (gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE)) {
		LOG_ERR("Button interrupt configure failed");
		return -EINVAL;
	}

	gpio_init_callback(&button_cb_data, button_pressed_cb, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	LOG_INF("Button configured correctly!");
	return 0;
}
/* ------------------------------------------------------------ */

/* GNSS config */

/**
 * @brief Callback function for GNSS data
 * @param dev - device pointer
 * @param data - GNSS data pointer
 */

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data)
{
	if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
		LOG_INF("GNSS FIX: %.6f, %.6f | Alt: %d.%dm | Sats: %d",
			data->nav_data.latitude / 1000000.0, data->nav_data.longitude / 1000000.0,
			data->nav_data.altitude / 1000, data->nav_data.altitude % 1000,
			data->info.satellites_cnt);
	} else {
		LOG_INF("GNSS: No fix | Sats: %d", data->info.satellites_cnt);
	}
}

GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(gnss)), gnss_data_cb);
/** ------------------------------------------------------------ */

/* main function */
int main(void)
{
	LOG_INF("Hello World");
	button_config();
	adc_config();
	tft_display_test();
	lora_init();

	if (lora_send_message_waveshare_dtu("Initializing T114\n\r")) {
		LOG_ERR("Failed to send LoRa message");
		return -EINVAL;
	}

	LOG_INF("============ Starting main loop ============");
	while (1) {
		int ret = 0;

		static int64_t last_adc = 0;
		static int64_t last_lora = 0;

		int64_t now = k_uptime_get();

		if (now - last_adc > ADC_TIME_INTERVAL) {
			ret = adc_read(adc_dev, &sequence);
			if (ret == 0) {
				LOG_INF("ADC AIN2 Value: %d", sample_buffer[0]);
			} else {
				LOG_ERR("ADC read failed: %d", ret);
			}
			last_adc = now;
		}

		if (now - last_lora > LORA_TIME_INTERVAL) {
			char msg[64];
			snprintf(msg, sizeof(msg), "ADC Value: %d", sample_buffer[0]);
			lora_send_message_waveshare_dtu(msg);
			if (ret) {
				LOG_ERR("Failed to send LoRa message: %d", ret);
			}
			last_lora = now;
		}
		k_msleep(10);
	}
	return 0;
}
