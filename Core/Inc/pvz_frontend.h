#ifndef PVZ_FRONTEND
#define PVZ_FRONTEND

#include "game.h"
#include "input.h"
#include "presentation.h"
#include "pvz_config.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	PlantType observed_piece[PVZ_MAX_ROWS][PVZ_MAX_COLS];
	bool hand_present;
} PvzFrontendSnapshot;

typedef enum {
	PVZ_FRONTEND_PENDING_NONE = 0,
	PVZ_FRONTEND_PENDING_PLACE,
	PVZ_FRONTEND_PENDING_REMOVE,
} PvzFrontendPendingActionType;

typedef struct {
	bool flash_active;
	RenderPalette flash_palette;
	float flash_timer_sec;
	bool remove_required;
} PvzFrontendTileState;

typedef struct {
	const GameConfig *config;
	PvzFrontendSnapshot raw_snapshot;
	PlantType stable_board[PVZ_MAX_ROWS][PVZ_MAX_COLS];
	PlantType debounce_candidate[PVZ_MAX_ROWS][PVZ_MAX_COLS];
	uint8_t debounce_count[PVZ_MAX_ROWS][PVZ_MAX_COLS];
	bool tile_armed[PVZ_MAX_ROWS][PVZ_MAX_COLS];
	PvzFrontendTileState tiles[PVZ_MAX_ROWS][PVZ_MAX_COLS];
	bool hand_present;
	bool hand_collect_pending;
	bool sync_blocked;
	uint32_t sync_blocked_until_ms;
	PvzFrontendPendingActionType pending_type;
	BoardCoord pending_coord;
	PlantType pending_plant_type;
} PvzFrontendState;

void pvz_frontend_init(PvzFrontendState *state, const GameConfig *config);
void pvz_frontend_fill_stub_snapshot(PvzFrontendSnapshot *snapshot, const GameConfig *config);
void pvz_frontend_fill_scripted_stub_snapshot(PvzFrontendSnapshot *snapshot, const GameConfig *config, uint32_t now_ms);
void pvz_frontend_ingest_snapshot(PvzFrontendState *state, const PvzFrontendSnapshot *snapshot, uint32_t now_ms);
void pvz_frontend_build_input(PvzFrontendState *state, const GameState *game, InputFrame *input, bool play_scene_active);
void pvz_frontend_post_update(PvzFrontendState *state, const GameState *previous, const GameState *current, float frame_dt,
							  bool play_scene_active);
void pvz_frontend_export_presentation_state(const PvzFrontendState *state, const GameState *game,
											PlayPresentationState *out_state);

#endif
