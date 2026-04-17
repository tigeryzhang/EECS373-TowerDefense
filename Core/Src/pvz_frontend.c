#include "pvz_frontend.h"

#include <string.h>

enum {
	PVZ_FRONTEND_DEBOUNCE_FRAMES = 3,
	PVZ_FRONTEND_HAND_GRACE_MS = 1000,
};

static const float PVZ_FRONTEND_FLASH_SUCCESS_SEC = 0.4f;
static const float PVZ_FRONTEND_FLASH_WARNING_SEC = 0.75f;

static PlantType normalize_piece_type(PlantType type) {
	switch (type) {
	case PLANT_SUNFLOWER:
	case PLANT_PEASHOOTER:
	case PLANT_WALLNUT:
		return type;
	case PLANT_NONE:
	default:
		return PLANT_NONE;
	}
}

static bool deadline_is_in_future(uint32_t now_ms, uint32_t deadline_ms) {
	return (int32_t)(deadline_ms - now_ms) > 0;
}

static bool coord_in_bounds(const GameConfig *config, BoardCoord coord) {
	return config && coord.row >= 0 && coord.row < config->rows && coord.col >= 0 && coord.col < config->cols;
}

static int plant_card_index(PlantType type) {
	switch (type) {
	case PLANT_SUNFLOWER:
		return 0;
	case PLANT_PEASHOOTER:
		return 1;
	case PLANT_WALLNUT:
		return 2;
	case PLANT_NONE:
	default:
		return -1;
	}
}

static PlantType game_plant_type_at(const GameState *game, BoardCoord coord) {
	if (!game || !coord_in_bounds(game->config, coord)) {
		return PLANT_NONE;
	}

	const int plant_index = game->plant_grid[coord.row][coord.col];
	if (plant_index < 0 || plant_index >= PVZ_MAX_PLANTS) {
		return PLANT_NONE;
	}
	if (!game->plants[plant_index].active) {
		return PLANT_NONE;
	}

	return game->plants[plant_index].type;
}

static void start_tile_flash(PvzFrontendState *state, BoardCoord coord, RenderPalette palette, float duration_sec) {
	if (!state || !coord_in_bounds(state->config, coord)) {
		return;
	}

	PvzFrontendTileState *tile = &state->tiles[coord.row][coord.col];
	tile->flash_active = true;
	tile->flash_palette = palette;
	tile->flash_timer_sec = duration_sec;
}

static void clear_pending_action(PvzFrontendState *state) {
	if (!state) {
		return;
	}

	state->pending_type = PVZ_FRONTEND_PENDING_NONE;
	state->pending_coord = (BoardCoord){0, 0};
	state->pending_plant_type = PLANT_NONE;
}

static void disarm_tile(PvzFrontendState *state, BoardCoord coord) {
	if (!state || !coord_in_bounds(state->config, coord)) {
		return;
	}

	state->tile_armed[coord.row][coord.col] = false;
}

static void refresh_tile_latches(PvzFrontendState *state) {
	if (!state || !state->config) {
		return;
	}

	for (int row = 0; row < state->config->rows; ++row) {
		for (int col = 0; col < state->config->cols; ++col) {
			if (state->stable_board[row][col] != PLANT_NONE) {
				continue;
			}
			state->tile_armed[row][col] = true;
			state->tiles[row][col].remove_required = false;
		}
	}
}

static void advance_flash_timers(PvzFrontendState *state, float frame_dt) {
	if (!state || !state->config) {
		return;
	}

	for (int row = 0; row < state->config->rows; ++row) {
		for (int col = 0; col < state->config->cols; ++col) {
			PvzFrontendTileState *tile = &state->tiles[row][col];
			if (!tile->flash_active) {
				continue;
			}

			tile->flash_timer_sec -= frame_dt;
			if (tile->flash_timer_sec <= 0.0f) {
				tile->flash_active = false;
				tile->flash_timer_sec = 0.0f;
			}
		}
	}
}

static void note_game_driven_removal(PvzFrontendState *state, BoardCoord coord) {
	if (!state || !coord_in_bounds(state->config, coord)) {
		return;
	}

	start_tile_flash(state, coord, RENDER_PALETTE_WARNING, PVZ_FRONTEND_FLASH_WARNING_SEC);
	disarm_tile(state, coord);
	state->tiles[coord.row][coord.col].remove_required = state->stable_board[coord.row][coord.col] != PLANT_NONE;
}

static void handle_pending_place_result(PvzFrontendState *state, GameCommandResult result) {
	const BoardCoord coord = state->pending_coord;

	switch (result) {
	case GAME_COMMAND_RESULT_OK:
		disarm_tile(state, coord);
		state->tiles[coord.row][coord.col].remove_required = false;
		start_tile_flash(state, coord, RENDER_PALETTE_SUCCESS, PVZ_FRONTEND_FLASH_SUCCESS_SEC);
		break;
	case GAME_COMMAND_RESULT_NOT_ENOUGH_SUN:
		start_tile_flash(state, coord, RENDER_PALETTE_WARNING, PVZ_FRONTEND_FLASH_WARNING_SEC);
		break;
	case GAME_COMMAND_RESULT_PLANT_ON_COOLDOWN:
	case GAME_COMMAND_RESULT_OCCUPIED:
	case GAME_COMMAND_RESULT_OUT_OF_BOUNDS:
	case GAME_COMMAND_RESULT_NO_SELECTION:
	case GAME_COMMAND_RESULT_NOT_FOUND:
	case GAME_COMMAND_RESULT_IGNORED:
	default:
		start_tile_flash(state, coord, RENDER_PALETTE_HIGHLIGHT, PVZ_FRONTEND_FLASH_WARNING_SEC);
		break;
	}
}

static void handle_pending_remove_result(PvzFrontendState *state, GameCommandResult result) {
	const BoardCoord coord = state->pending_coord;

	switch (result) {
	case GAME_COMMAND_RESULT_OK:
		disarm_tile(state, coord);
		state->tiles[coord.row][coord.col].remove_required = false;
		start_tile_flash(state, coord, RENDER_PALETTE_ART_3, PVZ_FRONTEND_FLASH_SUCCESS_SEC);
		break;
	case GAME_COMMAND_RESULT_NOT_FOUND:
	case GAME_COMMAND_RESULT_OUT_OF_BOUNDS:
	case GAME_COMMAND_RESULT_IGNORED:
	default:
		start_tile_flash(state, coord, RENDER_PALETTE_HIGHLIGHT, PVZ_FRONTEND_FLASH_WARNING_SEC);
		break;
	}
}

void pvz_frontend_init(PvzFrontendState *state, const GameConfig *config) {
	if (!state) {
		return;
	}

	memset(state, 0, sizeof(*state));
	state->config = config;
	state->pending_coord = (BoardCoord){0, 0};
	state->pending_plant_type = PLANT_NONE;
	for (int row = 0; row < PVZ_MAX_ROWS; ++row) {
		for (int col = 0; col < PVZ_MAX_COLS; ++col) {
			state->stable_board[row][col] = PLANT_NONE;
			state->debounce_candidate[row][col] = PLANT_NONE;
			state->tile_armed[row][col] = true;
			state->tiles[row][col].flash_palette = RENDER_PALETTE_SUCCESS;
		}
	}
}

void pvz_frontend_fill_stub_snapshot(PvzFrontendSnapshot *snapshot, const GameConfig *config) {
	if (!snapshot) {
		return;
	}

	memset(snapshot, 0, sizeof(*snapshot));
	if (!config || config->rows <= 0 || config->cols <= 0) {
		return;
	}

	BoardCoord sunflower = {0, 0};
	BoardCoord peashooter = {config->rows > 1 ? 1 : 0, config->cols > 1 ? 1 : 0};
	BoardCoord wallnut = {config->rows > 2 ? 2 : config->rows - 1, config->cols > 2 ? 2 : config->cols - 1};

	snapshot->observed_piece[sunflower.row][sunflower.col] = PLANT_SUNFLOWER;
	if (snapshot->observed_piece[peashooter.row][peashooter.col] == PLANT_NONE) {
		snapshot->observed_piece[peashooter.row][peashooter.col] = PLANT_PEASHOOTER;
	}
	if (snapshot->observed_piece[wallnut.row][wallnut.col] == PLANT_NONE) {
		snapshot->observed_piece[wallnut.row][wallnut.col] = PLANT_WALLNUT;
	}
}

void pvz_frontend_ingest_snapshot(PvzFrontendState *state, const PvzFrontendSnapshot *snapshot, uint32_t now_ms) {
	if (!state || !state->config || !snapshot) {
		return;
	}

	state->raw_snapshot.hand_present = snapshot->hand_present;
	state->hand_present = snapshot->hand_present;
	if (snapshot->hand_present) {
		state->sync_blocked_until_ms = now_ms + PVZ_FRONTEND_HAND_GRACE_MS;
	}
	state->sync_blocked = snapshot->hand_present || deadline_is_in_future(now_ms, state->sync_blocked_until_ms);

	for (int row = 0; row < state->config->rows; ++row) {
		for (int col = 0; col < state->config->cols; ++col) {
			const PlantType observed = normalize_piece_type(snapshot->observed_piece[row][col]);
			state->raw_snapshot.observed_piece[row][col] = observed;

			if (state->sync_blocked) {
				state->debounce_candidate[row][col] = observed;
				state->debounce_count[row][col] = 0;
				continue;
			}

			if (state->debounce_candidate[row][col] != observed) {
				state->debounce_candidate[row][col] = observed;
				state->debounce_count[row][col] = 1;
			} else if (state->debounce_count[row][col] < UINT8_MAX) {
				state->debounce_count[row][col]++;
			}

			if (state->stable_board[row][col] != observed &&
				state->debounce_count[row][col] >= PVZ_FRONTEND_DEBOUNCE_FRAMES) {
				state->stable_board[row][col] = observed;
			}
		}
	}

	refresh_tile_latches(state);
}

void pvz_frontend_build_input(PvzFrontendState *state, const GameState *game, InputFrame *input, bool play_scene_active) {
	if (!state || !game || !input || !state->config || !play_scene_active) {
		return;
	}
	if (state->sync_blocked || state->pending_type != PVZ_FRONTEND_PENDING_NONE) {
		return;
	}

	for (int row = 0; row < state->config->rows; ++row) {
		for (int col = 0; col < state->config->cols; ++col) {
			const BoardCoord coord = {row, col};
			const PlantType stable_piece = state->stable_board[row][col];
			const PlantType live_plant = game_plant_type_at(game, coord);
			if (state->tiles[row][col].flash_active) {
				continue;
			}
			if (stable_piece == PLANT_NONE && live_plant != PLANT_NONE) {
				if (input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_REMOVE_TILE, .coord = coord})) {
					state->pending_type = PVZ_FRONTEND_PENDING_REMOVE;
					state->pending_coord = coord;
					state->pending_plant_type = live_plant;
				}
				return;
			}
		}
	}

	for (int row = 0; row < state->config->rows; ++row) {
		for (int col = 0; col < state->config->cols; ++col) {
			const BoardCoord coord = {row, col};
			const PlantType stable_piece = state->stable_board[row][col];
			const PlantType live_plant = game_plant_type_at(game, coord);
			const int card_index = plant_card_index(stable_piece);
			if (state->tiles[row][col].flash_active) {
				continue;
			}

			if (stable_piece == PLANT_NONE || live_plant != PLANT_NONE || !state->tile_armed[row][col] || card_index < 0) {
				continue;
			}

			if (!input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_SELECT_CARD, .index = card_index})) {
				return;
			}
			if (!input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_PLACE_TILE, .coord = coord})) {
				input->count--;
				return;
			}

			state->pending_type = PVZ_FRONTEND_PENDING_PLACE;
			state->pending_coord = coord;
			state->pending_plant_type = stable_piece;
			return;
		}
	}
}

void pvz_frontend_post_update(PvzFrontendState *state, const GameState *previous, const GameState *current, float frame_dt,
							  bool play_scene_active) {
	if (!state || !state->config) {
		return;
	}

	advance_flash_timers(state, frame_dt);
	refresh_tile_latches(state);

	if (!play_scene_active || !previous || !current) {
		return;
	}

	const PvzFrontendPendingActionType pending_type = state->pending_type;
	const BoardCoord pending_coord = state->pending_coord;
	const bool pending_remove_succeeded =
		pending_type == PVZ_FRONTEND_PENDING_REMOVE && current->last_command_result == GAME_COMMAND_RESULT_OK;

	if (pending_type == PVZ_FRONTEND_PENDING_PLACE) {
		handle_pending_place_result(state, current->last_command_result);
		clear_pending_action(state);
	} else if (pending_type == PVZ_FRONTEND_PENDING_REMOVE) {
		handle_pending_remove_result(state, current->last_command_result);
		clear_pending_action(state);
	}

	for (int row = 0; row < state->config->rows; ++row) {
		for (int col = 0; col < state->config->cols; ++col) {
			const BoardCoord coord = {row, col};
			const PlantType previous_type = game_plant_type_at(previous, coord);
			const PlantType current_type = game_plant_type_at(current, coord);
			const bool same_as_pending_remove = pending_remove_succeeded && coord.row == pending_coord.row &&
												coord.col == pending_coord.col;

			if (previous_type != PLANT_NONE && current_type == PLANT_NONE && !same_as_pending_remove) {
				note_game_driven_removal(state, coord);
			}
		}
	}

	refresh_tile_latches(state);
}

void pvz_frontend_export_presentation_state(const PvzFrontendState *state, const GameState *game,
											PlayPresentationState *out_state) {
	if (!state || !out_state) {
		return;
	}

	memset(out_state, 0, sizeof(*out_state));
	out_state->physical_mode_enabled = true;
	out_state->suppress_card_selection = true;

	if (!state->config || !game) {
		return;
	}

	for (int row = 0; row < state->config->rows; ++row) {
		for (int col = 0; col < state->config->cols; ++col) {
			const BoardCoord coord = {row, col};
			const PlantType live_plant = game_plant_type_at(game, coord);
			const PlantType stable_piece = state->stable_board[row][col];
			BoardTilePresentationState *tile = &out_state->tiles[row][col];
			const PvzFrontendTileState *frontend_tile = &state->tiles[row][col];

			tile->flash_active = frontend_tile->flash_active;
			tile->flash_palette = frontend_tile->flash_palette;
			tile->remove_required = frontend_tile->remove_required;

			if (tile->remove_required) {
				continue;
			}
			if (live_plant == PLANT_NONE) {
				continue;
			}
			if (stable_piece == live_plant) {
				tile->valid_corners = true;
			} else if (stable_piece != PLANT_NONE) {
				tile->mismatch_warning = true;
			}
		}
	}
}
