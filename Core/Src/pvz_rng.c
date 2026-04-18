#include "pvz_rng.h"

#include <stdint.h>

#define PVZ_RNG_MULTIPLIER 1664525u
#define PVZ_RNG_INCREMENT 1013904223u
#define PVZ_RNG_INITIAL_STATE 0xA341316Cu

float get_random() {
	static uint32_t state = PVZ_RNG_INITIAL_STATE;

	state = state * PVZ_RNG_MULTIPLIER + PVZ_RNG_INCREMENT;
	return (float)state / (float)UINT32_MAX;
}
