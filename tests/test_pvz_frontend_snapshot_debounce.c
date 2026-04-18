#include "pvz_frontend.h"

#include <assert.h>
#include <stdio.h>

static GameConfig make_test_config(void) {
	GameConfig config = {0};
	config.rows = 4;
	config.cols = 7;
	return config;
}

static void expect_tile(const PvzFrontendState *state, int row, int col, PlantType expected) {
	assert(state != NULL);
	assert(row >= 0 && row < PVZ_MAX_ROWS);
	assert(col >= 0 && col < PVZ_MAX_COLS);
	assert(state->stable_board[row][col] == expected);
}

static void test_debounce_requires_fresh_snapshots(void) {
	GameConfig config = make_test_config();
	PvzFrontendState state;
	PvzFrontendSnapshot snapshot = {0};

	pvz_frontend_init(&state, &config);

	snapshot.observed_piece[0][0] = PLANT_SUNFLOWER;

	pvz_frontend_ingest_snapshot(&state, &snapshot, 0u, true);
	expect_tile(&state, 0, 0, PLANT_NONE);

	for (uint32_t now_ms = 50u; now_ms <= 500u; now_ms += 50u) {
		pvz_frontend_ingest_snapshot(&state, &snapshot, now_ms, false);
	}
	expect_tile(&state, 0, 0, PLANT_NONE);

	pvz_frontend_ingest_snapshot(&state, &snapshot, 550u, true);
	expect_tile(&state, 0, 0, PLANT_SUNFLOWER);
}

static void test_hand_grace_requires_post_grace_snapshots(void) {
	GameConfig config = make_test_config();
	PvzFrontendState state;
	PvzFrontendSnapshot placed_snapshot = {0};
	PvzFrontendSnapshot hand_snapshot = {0};
	PvzFrontendSnapshot cleared_snapshot = {0};

	pvz_frontend_init(&state, &config);

	placed_snapshot.observed_piece[1][1] = PLANT_PEASHOOTER;
	pvz_frontend_ingest_snapshot(&state, &placed_snapshot, 0u, true);
	pvz_frontend_ingest_snapshot(&state, &placed_snapshot, 500u, true);
	expect_tile(&state, 1, 1, PLANT_PEASHOOTER);

	hand_snapshot.hand_present = true;
	pvz_frontend_ingest_snapshot(&state, &hand_snapshot, 1000u, true);
	expect_tile(&state, 1, 1, PLANT_PEASHOOTER);

	for (uint32_t now_ms = 1050u; now_ms < 2000u; now_ms += 50u) {
		pvz_frontend_ingest_snapshot(&state, &cleared_snapshot, now_ms, false);
	}
	expect_tile(&state, 1, 1, PLANT_PEASHOOTER);

	pvz_frontend_ingest_snapshot(&state, &cleared_snapshot, 2000u, true);
	expect_tile(&state, 1, 1, PLANT_PEASHOOTER);

	pvz_frontend_ingest_snapshot(&state, &cleared_snapshot, 2500u, true);
	expect_tile(&state, 1, 1, PLANT_NONE);
}

int main(void) {
	test_debounce_requires_fresh_snapshots();
	test_hand_grace_requires_post_grace_snapshots();
	puts("pvz_frontend snapshot debounce tests passed");
	return 0;
}
