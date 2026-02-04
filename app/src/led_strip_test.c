#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>

#include "board_init.h"
#include "led_strip_test.h"

LOG_MODULE_REGISTER(led_strip_test, LOG_LEVEL_DBG);

#define COLOR_OFF   {.r = 0x00, .g = 0x00, .b = 0x00}
#define COLOR_RED   {.r = 0xFF, .g = 0x00, .b = 0x00}
#define COLOR_GREEN {.r = 0x00, .g = 0xFF, .b = 0x00}
#define COLOR_BLUE  {.r = 0x00, .g = 0x00, .b = 0xFF}
#define COLOR_WHITE {.r = 0xFF, .g = 0xFF, .b = 0xFF}

struct led_rgb colors[] = {
	COLOR_OFF, COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_WHITE, COLOR_OFF,
};

uint8_t pixel0_index = 0;
uint8_t pixel1_index = 1;

/**
 * @brief Rotate the LED strip pixels
 * @return 0 on success, 1 on failure
 */

int sk6812_pixel_rotate()
{
	struct led_rgb pixels[2] = {colors[pixel0_index], colors[pixel1_index]};

	pixel0_index = (pixel0_index + 1) % ARRAY_SIZE(colors);
	pixel1_index = (pixel1_index + 1) % ARRAY_SIZE(colors);

	int ret = led_strip_update_rgb(led_strip, pixels, 2);
	if (ret) {
		LOG_ERR("LED update failed: %d", ret);
		return 1;
	}
	return 0;
}
