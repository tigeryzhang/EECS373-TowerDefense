#include "hub75.h"

#include "hub75_target.h"

#include <stdbool.h>
#include <string.h>

#define HUB75_BUFFER_COUNT 2u
#define HUB75_HALF_HEIGHT (HUB75_HEIGHT / 2u)
#define HUB75_COLOR_MAX ((1u << HUB75_COLOR_BITS) - 1u)
#define HUB75_DATA_MASK (HUB75_R1_PIN | HUB75_G1_PIN | HUB75_B1_PIN | HUB75_R2_PIN | HUB75_G2_PIN | HUB75_B2_PIN)
#define HUB75_ADDR_MASK (HUB75_A_PIN | HUB75_B_PIN | HUB75_C_PIN | HUB75_D_PIN)

#if defined(__GNUC__)
#define HUB75_RAM3_BSS __attribute__((section(".ram3_bss")))
#else
#define HUB75_RAM3_BSS
#endif

static uint32_t row_addr_bsrr[HUB75_SCAN_ROWS];
static uint16_t upper_color_masks[256][HUB75_COLOR_BITS];
static uint16_t lower_color_masks[256][HUB75_COLOR_BITS];
static HUB75_RAM3_BSS uint32_t scan_words[HUB75_BUFFER_COUNT][HUB75_SCAN_ROWS][HUB75_COLOR_BITS][HUB75_WIDTH];

static volatile uint8_t active_buffer_index;
static volatile uint8_t pending_buffer_index;
static volatile bool pending_swap;
static volatile bool running;
static uint8_t current_row;
static uint8_t current_plane;

static inline uint32_t data_word_from_masks(uint16_t upper_mask, uint16_t lower_mask) {
	return ((uint32_t)HUB75_DATA_MASK << 16) | (uint32_t)(upper_mask | lower_mask);
}

static inline uint32_t addr_word_for_row(uint8_t row) {
	uint32_t bsrr = (uint32_t)HUB75_ADDR_MASK << 16;

	if ((row & 0x01u) != 0u) {
		bsrr |= HUB75_A_PIN;
	}
	if ((row & 0x02u) != 0u) {
		bsrr |= HUB75_B_PIN;
	}
	if ((row & 0x04u) != 0u) {
		bsrr |= HUB75_C_PIN;
	}
	if ((row & 0x08u) != 0u) {
		bsrr |= HUB75_D_PIN;
	}

	return bsrr;
}

static inline uint8_t quantize_channel(uint8_t value, uint8_t input_max) {
	return (uint8_t)(((uint32_t)value * HUB75_COLOR_MAX + (uint32_t)(input_max / 2u)) / (uint32_t)input_max);
}

static inline uint8_t quantize_5bit(uint8_t value) {
	return quantize_channel(value, 31u);
}

static inline uint8_t quantize_6bit(uint8_t value) {
	return quantize_channel(value, 63u);
}

static inline void set_output_blank(bool blank) {
	HUB75_CTRL_PORT->BSRR = blank ? HUB75_OE_PIN : ((uint32_t)HUB75_OE_PIN << 16);
}

static inline void pulse_latch(void) {
	HUB75_CTRL_PORT->BSRR = HUB75_LAT_PIN;
	HUB75_CTRL_PORT->BSRR = (uint32_t)HUB75_LAT_PIN << 16;
}

static inline void pulse_clock(void) {
	HUB75_DATA_PORT->BSRR = HUB75_CLK_PIN;
	HUB75_DATA_PORT->BSRR = (uint32_t)HUB75_CLK_PIN << 16;
}

static void init_row_addr_table(void) {
	for (uint8_t row = 0; row < HUB75_SCAN_ROWS; ++row) {
		row_addr_bsrr[row] = addr_word_for_row(row);
	}
}

static void clear_scan_buffers(void) {
	const uint32_t blank_word = (uint32_t)HUB75_DATA_MASK << 16;

	for (uint8_t buffer = 0; buffer < HUB75_BUFFER_COUNT; ++buffer) {
		for (uint8_t row = 0; row < HUB75_SCAN_ROWS; ++row) {
			for (uint8_t plane = 0; plane < HUB75_COLOR_BITS; ++plane) {
				for (uint8_t col = 0; col < HUB75_WIDTH; ++col) {
					scan_words[buffer][row][plane][col] = blank_word;
				}
			}
		}
	}
}

static void set_palette_masks(uint8_t index, uint8_t red_level, uint8_t green_level, uint8_t blue_level) {
	for (uint8_t plane = 0; plane < HUB75_COLOR_BITS; ++plane) {
		const uint8_t plane_bit = (uint8_t)(1u << plane);
		const bool red_on = (red_level & plane_bit) != 0u;
		const bool green_on = (green_level & plane_bit) != 0u;
		const bool blue_on = (blue_level & plane_bit) != 0u;
		const bool upper_green = HUB75_SWAP_GB ? blue_on : green_on;
		const bool upper_blue = HUB75_SWAP_GB ? green_on : blue_on;

		uint16_t upper_mask = 0u;
		uint16_t lower_mask = 0u;

		if (red_on) {
			upper_mask |= HUB75_R1_PIN;
			lower_mask |= HUB75_R2_PIN;
		}
		if (upper_green) {
			upper_mask |= HUB75_G1_PIN;
			lower_mask |= HUB75_G2_PIN;
		}
		if (upper_blue) {
			upper_mask |= HUB75_B1_PIN;
			lower_mask |= HUB75_B2_PIN;
		}

		upper_color_masks[index][plane] = upper_mask;
		lower_color_masks[index][plane] = lower_mask;
	}
}

static void shift_row_words(const uint32_t *words) {
	for (uint8_t col = 0; col < HUB75_WIDTH; ++col) {
		HUB75_DATA_PORT->BSRR = words[col];
		pulse_clock();
	}
}

static void advance_refresh_state(void) {
	++current_plane;
	if (current_plane >= HUB75_COLOR_BITS) {
		current_plane = 0u;
		++current_row;
		if (current_row >= HUB75_SCAN_ROWS) {
			current_row = 0u;
		}
	}
}

static void set_plane_period(uint8_t plane) {
	const uint32_t ticks = (uint32_t)(HUB75_TIMER_BASE_PERIOD + 1u) << plane;
	__HAL_TIM_SET_AUTORELOAD(&htim15, ticks - 1u);
}

static inline void clear_timer_update_flag(void) {
	htim15.Instance->SR = (uint32_t)~((uint32_t)TIM_FLAG_UPDATE);
}

void hub75_init(void) {
	running = false;
	pending_swap = false;
	active_buffer_index = 0u;
	pending_buffer_index = 0u;
	current_row = 0u;
	current_plane = 0u;

	init_row_addr_table();
	clear_scan_buffers();
	memset(upper_color_masks, 0, sizeof(upper_color_masks));
	memset(lower_color_masks, 0, sizeof(lower_color_masks));

	HUB75_DATA_PORT->BSRR = ((uint32_t)(HUB75_DATA_MASK | HUB75_CLK_PIN) << 16);
	HUB75_ADDR_PORT->BSRR = (uint32_t)HUB75_ADDR_MASK << 16;
	HUB75_CTRL_PORT->BSRR = ((uint32_t)HUB75_LAT_PIN << 16) | HUB75_OE_PIN;

	set_plane_period(0u);
	__HAL_TIM_SET_COUNTER(&htim15, 0u);
}

void hub75_start(void) {
	if (running) {
		return;
	}

	running = true;
	current_row = 0u;
	current_plane = 0u;

	hub75_irq_handler();
	__HAL_TIM_SET_COUNTER(&htim15, 0u);
	clear_timer_update_flag();
	HAL_TIM_Base_Start_IT(&htim15);
}

void hub75_stop(void) {
	running = false;
	HAL_TIM_Base_Stop_IT(&htim15);
	set_output_blank(true);
}

void hub75_set_palette_rgb565(uint8_t index, uint16_t rgb565) {
	const uint8_t red5 = (uint8_t)((rgb565 >> 11) & 0x1Fu);
	const uint8_t green6 = (uint8_t)((rgb565 >> 5) & 0x3Fu);
	const uint8_t blue5 = (uint8_t)(rgb565 & 0x1Fu);

	set_palette_masks(index, quantize_5bit(red5), quantize_6bit(green6), quantize_5bit(blue5));
}

void hub75_upload_indexed_64x32(const uint8_t *pixels, uint16_t stride) {
	if (pixels == NULL || stride < HUB75_WIDTH) {
		return;
	}

	while (running && pending_swap) {
		__NOP();
	}

	const uint8_t write_buffer = (uint8_t)(active_buffer_index ^ 1u);

	for (uint8_t row = 0; row < HUB75_SCAN_ROWS; ++row) {
		const uint8_t *top_row = pixels + (uint32_t)row * stride;
		const uint8_t *bottom_row = pixels + (uint32_t)(row + HUB75_HALF_HEIGHT) * stride;

		for (uint8_t plane = 0; plane < HUB75_COLOR_BITS; ++plane) {
			for (uint8_t col = 0; col < HUB75_WIDTH; ++col) {
				const uint8_t top_index = top_row[col];
				const uint8_t bottom_index = bottom_row[col];

				scan_words[write_buffer][row][plane][col] =
					data_word_from_masks(upper_color_masks[top_index][plane], lower_color_masks[bottom_index][plane]);
			}
		}
	}

	if (!running) {
		active_buffer_index = write_buffer;
		pending_swap = false;
		return;
	}

	const uint32_t primask = __get_PRIMASK();
	__disable_irq();
	pending_buffer_index = write_buffer;
	pending_swap = true;
	if (primask == 0u) {
		__enable_irq();
	}
}

void hub75_irq_handler(void) {
	if (!running) {
		return;
	}

	if (pending_swap && current_row == 0u && current_plane == 0u) {
		active_buffer_index = pending_buffer_index;
		pending_swap = false;
	}

	set_output_blank(true);
	HUB75_ADDR_PORT->BSRR = row_addr_bsrr[current_row];
	shift_row_words(scan_words[active_buffer_index][current_row][current_plane]);
	pulse_latch();
	set_output_blank(false);
	set_plane_period(current_plane);
	advance_refresh_state();
}
