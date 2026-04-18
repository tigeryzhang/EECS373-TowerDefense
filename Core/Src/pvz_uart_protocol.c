#include "pvz_uart_protocol.h"

#include <string.h>

static bool config_is_valid(const GameConfig *config) {
	return config != NULL && config->rows > 0 && config->rows <= PVZ_MAX_ROWS && config->cols > 0 &&
		   config->cols <= PVZ_MAX_COLS;
}

static bool decode_piece_tag(uint8_t tag, PlantType *out_type) {
	if (out_type == NULL) {
		return false;
	}

	switch (tag) {
	case 0:
		*out_type = PLANT_SUNFLOWER;
		return true;
	case 1:
		*out_type = PLANT_PEASHOOTER;
		return true;
	case 2:
		*out_type = PLANT_WALLNUT;
		return true;
	default:
		*out_type = PLANT_NONE;
		return false;
	}
}

bool pvz_uart_protocol_decode_packet(const uint8_t *packet, uint16_t size, const GameConfig *config,
									 PvzFrontendSnapshot *out_snapshot) {
	PvzFrontendSnapshot decoded;
	bool occupied[PVZ_MAX_ROWS][PVZ_MAX_COLS];

	if (packet == NULL || out_snapshot == NULL || !config_is_valid(config) || size < 1u) {
		return false;
	}

	const uint16_t count = packet[0];
	if (count > (uint16_t)(config->rows * config->cols)) {
		return false;
	}
	if (size != (uint16_t)(1u + count * 3u)) {
		return false;
	}

	memset(&decoded, 0, sizeof(decoded));
	memset(occupied, 0, sizeof(occupied));

	for (uint16_t index = 0; index < count; ++index) {
		const uint16_t offset = (uint16_t)(1u + index * 3u);
		const uint8_t tag = packet[offset];
		const uint8_t col = packet[offset + 1u];
		const uint8_t row = packet[offset + 2u];
		PlantType plant_type = PLANT_NONE;

		if (row >= (uint8_t)config->rows || col >= (uint8_t)config->cols) {
			return false;
		}
		if (!decode_piece_tag(tag, &plant_type)) {
			return false;
		}
		if (occupied[row][col]) {
			return false;
		}

		occupied[row][col] = true;
		decoded.observed_piece[row][col] = plant_type;
	}

	decoded.hand_present = false;
	*out_snapshot = decoded;
	return true;
}
