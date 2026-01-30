#include <zephyr/drivers/display.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include "board_init.h"

LOG_MODULE_REGISTER(tft_display_test, LOG_LEVEL_DBG);

#define TEST_SCREEN_WAIT 200
#define TFT_WIDTH        DT_PROP(DT_NODELABEL(tft_display), width)
#define TFT_HEIGHT       DT_PROP(DT_NODELABEL(tft_display), height)

/**
 * @brief Clear the TFT display
 * @param color The color to clear the display with
 */
static void tft_clear(uint16_t color)
{
	struct display_buffer_descriptor desc;
	static uint16_t line_buf[TFT_WIDTH];

	for (int i = 0; i < TFT_WIDTH; i++) {
		line_buf[i] = color;
	}

	desc.width = TFT_WIDTH;
	desc.height = 1;
	desc.pitch = TFT_WIDTH;
	desc.buf_size = sizeof(line_buf);

	for (int y = 0; y < TFT_HEIGHT; y++) {
		display_write(tft_display_dev, 0, y, &desc, line_buf);
	}
}

/**
 * @brief Draw a border on the TFT display
 * @param color The color to draw the border with
 */
static void tft_draw_border(uint16_t color)
{
	uint16_t c = sys_cpu_to_be16(color);
	struct display_buffer_descriptor desc;

	static uint16_t h_line[TFT_WIDTH];
	for (int i = 0; i < TFT_WIDTH; i++) {
		h_line[i] = c;
	}

	desc.width = TFT_WIDTH;
	desc.height = 1;
	desc.pitch = TFT_WIDTH;
	desc.buf_size = sizeof(h_line);

	display_write(tft_display_dev, 0, 0, &desc, h_line);              // Top edge
	display_write(tft_display_dev, 0, TFT_HEIGHT - 1, &desc, h_line); // Bottom edge

	// Left and right vertical lines (1 pixel wide, full height)
	static uint16_t pixel[1];
	pixel[0] = c;

	desc.width = 1;
	desc.height = 1;
	desc.pitch = 1;
	desc.buf_size = sizeof(pixel);

	for (int y = 0; y < TFT_HEIGHT; y++) {
		display_write(tft_display_dev, 0, y, &desc, pixel);             // Left edge
		display_write(tft_display_dev, TFT_WIDTH - 1, y, &desc, pixel); // Right edge
	}
}

/**
 * @brief Draw corners on the TFT display
 */
static void tft_draw_corners(void)
{
	struct display_buffer_descriptor desc;
	static uint16_t block[10 * 10]; // 10x10 pixel blocks

	desc.width = 10;
	desc.height = 10;
	desc.pitch = 10;
	desc.buf_size = sizeof(block);

	// Top-left: Red
	for (int i = 0; i < 100; i++) {
		block[i] = sys_cpu_to_be16(0xF800);
	}
	display_write(tft_display_dev, 0, 0, &desc, block);

	// Top-right: Green
	for (int i = 0; i < 100; i++) {
		block[i] = sys_cpu_to_be16(0x07E0);
	}
	display_write(tft_display_dev, TFT_WIDTH - 10, 0, &desc, block); // TFT_WIDTH - 10 = 125

	// Bottom-left: Blue
	for (int i = 0; i < 100; i++) {
		block[i] = sys_cpu_to_be16(0x001F);
	}
	display_write(tft_display_dev, 0, TFT_HEIGHT - 10, &desc, block); // TFT_HEIGHT - 10 = 230

	// Bottom-right: White
	for (int i = 0; i < 100; i++) {
		block[i] = sys_cpu_to_be16(0xFFFF);
	}
	display_write(tft_display_dev, TFT_WIDTH - 10, TFT_HEIGHT - 10, &desc, block);
}

/**
 * @brief Test the TFT display
 */
void tft_display_test()
{
	LOG_INF("Display blanking off");
	display_blanking_off(tft_display_dev);
	k_msleep(TEST_SCREEN_WAIT);

	LOG_INF("Display black");
	tft_clear(sys_cpu_to_be16(0x0000));
	k_msleep(TEST_SCREEN_WAIT);

	LOG_INF("Display red");
	tft_clear(sys_cpu_to_be16(0xF800)); // Red
	k_msleep(TEST_SCREEN_WAIT);

	LOG_INF("Display green");
	tft_clear(sys_cpu_to_be16(0x07E0)); // Green
	k_msleep(TEST_SCREEN_WAIT);

	LOG_INF("Display blue");
	tft_clear(sys_cpu_to_be16(0x001F)); // Blue
	k_msleep(TEST_SCREEN_WAIT);

	LOG_INF("Display white");
	tft_clear(sys_cpu_to_be16(0xFFFF)); // White

	LOG_INF("Display black");
	tft_clear(sys_cpu_to_be16(0x0000));
	k_msleep(TEST_SCREEN_WAIT);

	LOG_INF("Display white border");
	tft_draw_border(0xFFFF);
	k_msleep(TEST_SCREEN_WAIT);

	LOG_INF("Display corner markers");
	tft_draw_corners();
	k_msleep(TEST_SCREEN_WAIT);
}
