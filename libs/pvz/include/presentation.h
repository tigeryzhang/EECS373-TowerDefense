#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "game_types.h"
#include "pvz_config.h"
#include "pvz_rect.h"

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} PaletteRgb;

typedef enum {
	RENDER_PALETTE_BG = 0,
	RENDER_PALETTE_BLACK,
	RENDER_PALETTE_PANEL,
	RENDER_PALETTE_TILE_LIGHT,
	RENDER_PALETTE_TILE_DARK,
	RENDER_PALETTE_HIGHLIGHT,
	RENDER_PALETTE_TEXT,
	RENDER_PALETTE_SUN,
	RENDER_PALETTE_PLANT,
	RENDER_PALETTE_WALLNUT,
	RENDER_PALETTE_ZOMBIE,
	RENDER_PALETTE_CONE,
	RENDER_PALETTE_BUCKET,
	RENDER_PALETTE_PROJECTILE,
	RENDER_PALETTE_WARNING,
	RENDER_PALETTE_SUCCESS,
	RENDER_PALETTE_ART_0,
	RENDER_PALETTE_ART_1,
	RENDER_PALETTE_ART_2,
	RENDER_PALETTE_ART_3,
	RENDER_PALETTE_ART_4,
	RENDER_PALETTE_ART_5,
	RENDER_PALETTE_ART_6,
	RENDER_PALETTE_ART_7,
	RENDER_PALETTE_ART_8,
} RenderPalette;

typedef enum {
	RENDER_STATUS_NONE = 0,
	RENDER_STATUS_PLACED,
	RENDER_STATUS_REMOVED,
	RENDER_STATUS_OCCUPIED,
	RENDER_STATUS_OUT_OF_BOUNDS,
	RENDER_STATUS_NO_SELECTION,
	RENDER_STATUS_NO_SUN,
	RENDER_STATUS_NOT_FOUND,
	RENDER_STATUS_RESET,
	RENDER_STATUS_PLACEHOLDER,
} RenderStatus;

typedef enum { RENDER_TARGET_BOARD = 0, RENDER_TARGET_HUD } RenderTarget;

#define MAX_DIRTY_RECTS 128
typedef struct {
	IntRect rects[MAX_DIRTY_RECTS];
	int count;
} DirtyRectList;

typedef struct {
	int board_width;
	int board_height;
	uint8_t board_pixels[PVZ_MAX_BOARD_PIXELS];
	DirtyRectList board_dirty_rects;
	int hud_width;
	int hud_height;
	uint8_t hud_pixels[PVZ_MAX_HUD_PIXELS];
	DirtyRectList hud_dirty_rects;
} RenderView;

typedef struct {
	PlantType selected_plant;
	float seed_cooldowns[3];
	int sun_count;
	bool paused;
	GameStatus game_status;
	float level_progress_01;
	uint8_t current_wave_index;
	uint8_t wave_count;
	bool wave_warning_active;
	bool current_wave_is_major;
	uint8_t flag_marker_count;
	uint8_t flag_marker_progress[PVZ_MAX_WAVE_FLAG_MARKERS];
	RenderStatus status;
} FrameData;

typedef struct {
	bool prev_frame_valid;
	FrameData prev_frame;
	FrameData frame;
} RenderData;

void render_view_init(RenderView *view, int board_width, int board_height, int hud_width, int hud_height);
void render_view_begin(RenderView *view);

void render_data_init(RenderData *data);
void render_data_update(RenderData *data, const GameState *game, RenderStatus status);
uint16_t presentation_palette_to_rgb565(RenderPalette palette);

void dirty_rect_list_clear(DirtyRectList *rects);

// Specific impls for each game scene
void presentation_prerender_play_view(RenderView *view, RenderData *data, const GameState *game);
void presentation_render_play_view(RenderView *view, RenderData *data, const GameState *game, RenderStatus status);
void presentation_render_placeholder_view(RenderView *view, const GameConfig *config);
