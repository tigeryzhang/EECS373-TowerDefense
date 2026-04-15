#include "presentation.h"
#include "game.h"
#include "game_types.h"
#include "pvz_config.h"
#include "pvz_rect.h"
#include "pvz_utils.h"
#include "render_assets.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const uint8_t digit_glyphs[10][5] = {
	{7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
	{7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
};

static const uint8_t letter_glyphs[26][5] = {
	{2, 5, 7, 5, 5}, {6, 5, 6, 5, 6}, {3, 4, 4, 4, 3}, {6, 5, 5, 5, 6}, {7, 4, 6, 4, 7}, {7, 4, 6, 4, 4},
	{3, 4, 5, 5, 3}, {5, 5, 7, 5, 5}, {7, 2, 2, 2, 7}, {1, 1, 1, 5, 2}, {5, 5, 6, 5, 5}, {4, 4, 4, 4, 7},
	{5, 7, 7, 5, 5}, {6, 5, 5, 5, 5}, {2, 5, 5, 5, 2}, {6, 5, 6, 4, 4}, {2, 5, 5, 7, 3}, {6, 5, 6, 5, 5},
	{3, 4, 2, 1, 6}, {7, 2, 2, 2, 2}, {5, 5, 5, 5, 7}, {5, 5, 5, 5, 2}, {5, 5, 7, 7, 5}, {5, 5, 2, 5, 5},
	{5, 5, 2, 2, 2}, {7, 1, 2, 4, 7},
};

// clang-format off
static const PaletteRgb palette_rgb[] = {
	[RENDER_PALETTE_BG] = {26, 43, 26},
	[RENDER_PALETTE_BLACK] = {0, 0, 0},
	[RENDER_PALETTE_PANEL] = {233, 223, 187},
	[RENDER_PALETTE_TILE_LIGHT] = {149, 196, 82},
	[RENDER_PALETTE_TILE_DARK] = {122, 168, 62},
	[RENDER_PALETTE_HIGHLIGHT] = {255, 208, 78},
	[RENDER_PALETTE_TEXT] = {49, 46, 37},
	[RENDER_PALETTE_SUN] = {255, 198, 48},
	[RENDER_PALETTE_PLANT] = {42, 137, 59},
	[RENDER_PALETTE_WALLNUT] = {141, 91, 46},
	[RENDER_PALETTE_ZOMBIE] = {105, 118, 110},
	[RENDER_PALETTE_CONE] = {235, 126, 34},
	[RENDER_PALETTE_BUCKET] = {137, 149, 160},
	[RENDER_PALETTE_PROJECTILE] = {80, 206, 68},
	[RENDER_PALETTE_WARNING] = {203, 72, 56},
	[RENDER_PALETTE_SUCCESS] = {65, 170, 95},
	[RENDER_PALETTE_ART_0] = {152, 170, 148},
	[RENDER_PALETTE_ART_1] = {64, 74, 66},
	[RENDER_PALETTE_ART_2] = {128, 93, 62},
	[RENDER_PALETTE_ART_3] = {84, 109, 138},
	[RENDER_PALETTE_ART_4] = {120, 40, 40},
	[RENDER_PALETTE_ART_5] = {61, 171, 87},
	[RENDER_PALETTE_ART_6] = {255, 161, 62},
	[RENDER_PALETTE_ART_7] = {169, 219, 109},
	[RENDER_PALETTE_ART_8] = {176, 121, 72},
};
// clang-format on

static IntRect target_bounds(const RenderView *view, RenderTarget target) {
	if (target == RENDER_TARGET_BOARD) {
		return pvz_rect_make(0, 0, view->board_width, view->board_height);
	}
	return pvz_rect_make(0, 0, view->hud_width, view->hud_height);
}

static DirtyRectList *dirty_rects_for_target(RenderView *view, RenderTarget target) {
	return target == RENDER_TARGET_BOARD ? &view->board_dirty_rects : &view->hud_dirty_rects;
}

static void mark_dirty_rect(RenderView *view, RenderTarget target, IntRect rect) {
	if (!view) {
		return;
	}

	rect = pvz_rect_intersection(rect, target_bounds(view, target));
	if (pvz_rect_is_empty(rect)) {
		return;
	}

	DirtyRectList *rects = dirty_rects_for_target(view, target);
	if (rects->count >= MAX_DIRTY_RECTS) {
		return;
	}
	rects->rects[rects->count++] = rect;
}

static void mark_full_target_dirty(RenderView *view, RenderTarget target) {
	mark_dirty_rect(view, target, target_bounds(view, target));
}

static IntRect board_cell_rect(const GameState *game, int row, int col, int padding) {
	const int x0 = col * game->config->board_x_resolution / game->config->cols;
	const int x1 = (col + 1) * game->config->board_x_resolution / game->config->cols;
	const int y0 = row * game->config->board_y_resolution / game->config->rows;
	const int y1 = (row + 1) * game->config->board_y_resolution / game->config->rows;
	IntRect rect = pvz_rect_make(x0, y0, x1 - x0, y1 - y0);
	if (padding > 0) {
		const int max_pad_x = rect.w > 1 ? (rect.w - 1) / 2 : 0;
		const int max_pad_y = rect.h > 1 ? (rect.h - 1) / 2 : 0;
		const int pad_x = padding < max_pad_x ? padding : max_pad_x;
		const int pad_y = padding < max_pad_y ? padding : max_pad_y;
		rect.x += pad_x;
		rect.y += pad_y;
		rect.w -= pad_x * 2;
		rect.h -= pad_y * 2;
	}
	return rect;
}

static IntRect board_entity_rect(const GameState *game, float row_center, float col_center, int size) {
	const int center_x = (int)lroundf(col_center * (float)game->config->board_x_resolution / (float)game->config->cols);
	const int center_y = (int)lroundf(row_center * (float)game->config->board_y_resolution / (float)game->config->rows);
	return pvz_rect_make(center_x - size / 2, center_y - size / 2, size, size);
}

static int board_unit_size(const GameConfig *config) {
	const int cell_width = config->board_x_resolution / config->cols;
	const int cell_height = config->board_y_resolution / config->rows;
	return cell_width < cell_height ? cell_width : cell_height;
}

static int text_width_3x5(const char *text, int scale) {
	int count = 0;
	while (text && text[count] != '\0') {
		++count;
	}
	return count > 0 ? count * scale * 4 - scale : 0;
}

static IntRect text_rect_3x5(const char *text, int x, int y, int scale) {
	return pvz_rect_make(x, y, text_width_3x5(text, scale), 5 * scale);
}

static IntRect determine_card_position(const RenderView *view, int index) {
	const int num_cards = 3;
	const int margin = 10;
	const int width = (view->hud_width - margin * (num_cards + 1)) / num_cards;
	const int x = margin + (width + margin) * index;
	return pvz_rect_make(x, 82, width, 180);
}

static IntRect cooldown_bar_rect(const RenderView *view, int index) {
	const IntRect card_rect = determine_card_position(view, index);
	return pvz_rect_make(card_rect.x - 5, card_rect.y, 3, card_rect.h);
}

static IntRect wave_panel_rect(const RenderView *view) { return pvz_rect_make(140, 8, view->hud_width - 150, 42); }

static IntRect wave_bar_outer_rect(const RenderView *view) {
	const IntRect panel = wave_panel_rect(view);
	return pvz_rect_make(panel.x + 10, panel.y + 18, panel.w - 20, 12);
}

static IntRect wave_bar_inner_rect(const RenderView *view) {
	const IntRect outer = wave_bar_outer_rect(view);
	return pvz_rect_make(outer.x + 1, outer.y + 1, outer.w - 2, outer.h - 2);
}

static IntRect wave_warning_rect(const RenderView *view) { return pvz_rect_make(140, 54, view->hud_width - 150, 18); }

static IntRect sun_value_origin_rect(const RenderView *view) {
	(void)view;
	return text_rect_3x5("", 10 + text_width_3x5("SUN: ", 3), 10, 3);
}

static bool render_view_contains(const RenderView *view, RenderTarget target, int x, int y) {
	if (!view) {
		return false;
	}
	const IntRect bounds = target_bounds(view, target);
	return x >= bounds.x && y >= bounds.y && x < bounds.x + bounds.w && y < bounds.y + bounds.h;
}

static void clear_target(RenderView *view, RenderTarget target, RenderPalette clear_color) {
	if (!view) {
		return;
	}

	switch (target) {
	case RENDER_TARGET_BOARD:
		memset(view->board_pixels, (unsigned char)clear_color, (size_t)view->board_width * (size_t)view->board_height);
		break;
	case RENDER_TARGET_HUD:
		memset(view->hud_pixels, (unsigned char)clear_color, (size_t)view->hud_width * (size_t)view->hud_height);
		break;
	}
}

static uint8_t *get_pixels(RenderView *view, RenderTarget target) {
	if (!view) {
		return NULL;
	}

	switch (target) {
	case RENDER_TARGET_BOARD:
		return view->board_pixels;
	case RENDER_TARGET_HUD:
		return view->hud_pixels;
	}

	return NULL;
}

static void set_pixel(RenderView *view, RenderTarget target, int x, int y, RenderPalette palette) {
	if (!render_view_contains(view, target, x, y)) {
		return;
	}

	uint8_t *pixels = get_pixels(view, target);
	const int width = target == RENDER_TARGET_BOARD ? view->board_width : view->hud_width;
	if (pixels == NULL) {
		return;
	}

	pixels[y * width + x] = (uint8_t)palette;
}

static void draw_rect_clipped(RenderView *view, RenderTarget target, IntRect rect, RenderPalette palette, int thickness,
							  const IntRect *clip) {
	IntRect draw_bounds;
	uint8_t *pixels;
	int width;

	if (!view || rect.w <= 0 || rect.h <= 0) {
		return;
	}

	draw_bounds = pvz_rect_intersection(rect, target_bounds(view, target));
	if (clip) {
		draw_bounds = pvz_rect_intersection(draw_bounds, *clip);
	}
	if (pvz_rect_is_empty(draw_bounds)) {
		return;
	}

	pixels = get_pixels(view, target);
	width = target == RENDER_TARGET_BOARD ? view->board_width : view->hud_width;
	if (pixels == NULL) {
		return;
	}

	for (int y = draw_bounds.y; y < draw_bounds.y + draw_bounds.h; ++y) {
		for (int x = draw_bounds.x; x < draw_bounds.x + draw_bounds.w; ++x) {
			if (thickness == 0 || y < rect.y + thickness || y >= rect.y + rect.h - thickness ||
				x < rect.x + thickness || x >= rect.x + rect.w - thickness) {
				pixels[y * width + x] = (uint8_t)palette;
			}
		}
	}
}

static void draw_rect(RenderView *view, RenderTarget target, IntRect rect, RenderPalette palette, int thickness) {
	draw_rect_clipped(view, target, rect, palette, thickness, NULL);
}

static void draw_sprite_clipped(RenderView *view, RenderTarget target, const RenderSprite *sprite, int x, int y,
								const IntRect *clip) {
	IntRect bounds;

	if (!view || !sprite || !sprite->pixels) {
		return;
	}

	bounds = pvz_rect_make(x, y, sprite->width, sprite->height);
	bounds = pvz_rect_intersection(bounds, target_bounds(view, target));
	if (clip) {
		bounds = pvz_rect_intersection(bounds, *clip);
	}
	if (pvz_rect_is_empty(bounds)) {
		return;
	}

	for (int row = bounds.y - y; row < bounds.y - y + bounds.h; ++row) {
		for (int col = bounds.x - x; col < bounds.x - x + bounds.w; ++col) {
			const uint8_t pixel = sprite->pixels[row * sprite->width + col];
			if (pixel == SPRITE_PIXEL_TRANSPARENT) {
				continue;
			}
			set_pixel(view, target, x + col, y + row, (RenderPalette)pixel);
		}
	}
}

static void draw_sprite(RenderView *view, RenderTarget target, const RenderSprite *sprite, int x, int y) {
	draw_sprite_clipped(view, target, sprite, x, y, NULL);
}

static void draw_sprite_scaled(RenderView *view, RenderTarget target, const RenderSprite *sprite, IntRect rect) {
	if (!view || !sprite || !sprite->pixels || rect.w <= 0 || rect.h <= 0) {
		return;
	}

	for (int y = 0; y < rect.h; ++y) {
		const int src_y = y * sprite->height / rect.h;
		for (int x = 0; x < rect.w; ++x) {
			const int src_x = x * sprite->width / rect.w;
			const uint8_t pixel = sprite->pixels[src_y * sprite->width + src_x];
			if (pixel == SPRITE_PIXEL_TRANSPARENT) {
				continue;
			}
			set_pixel(view, target, rect.x + x, rect.y + y, (RenderPalette)pixel);
		}
	}
}

static const uint8_t *glyph_rows_for_char(char c) {
	static const uint8_t space_glyph[5] = {0, 0, 0, 0, 0};
	static const uint8_t dash_glyph[5] = {0, 0, 7, 0, 0};
	static const uint8_t colon_glyph[5] = {0, 2, 0, 2, 0};

	if (c >= 'a' && c <= 'z') {
		c = (char)(c - 'a' + 'A');
	}
	if (c >= '0' && c <= '9') {
		return digit_glyphs[c - '0'];
	}
	if (c >= 'A' && c <= 'Z') {
		return letter_glyphs[c - 'A'];
	}
	if (c == '-') {
		return dash_glyph;
	}
	if (c == ':') {
		return colon_glyph;
	}
	return space_glyph;
}

static void draw_char_3x5(RenderView *view, RenderTarget target, char c, int x, int y, int scale,
						  RenderPalette palette) {
	const uint8_t *rows = glyph_rows_for_char(c);
	for (int row = 0; row < 5; ++row) {
		for (int col = 0; col < 3; ++col) {
			const int bit = 1 << (2 - col);
			if ((rows[row] & bit) == 0) {
				continue;
			}
			draw_rect(view, target, pvz_rect_make(x + col * scale, y + row * scale, scale, scale), palette, 0);
		}
	}
}

static void draw_text_3x5(RenderView *view, RenderTarget target, const char *text, int x, int y, int scale,
						  RenderPalette palette) {
	if (!text || scale <= 0) {
		return;
	}

	for (int i = 0; text[i] != '\0'; ++i) {
		draw_char_3x5(view, target, text[i], x + i * scale * 4, y, scale, palette);
	}
}

static void draw_tile_checkerboard(RenderView *view, const GameState *game, const IntRect *clip) {
	for (int row = 0; row < game->config->rows; ++row) {
		for (int col = 0; col < game->config->cols; ++col) {
			const IntRect rect = board_cell_rect(game, row, col, 0);
			const RenderPalette palette = ((row + col) % 2 == 0) ? RENDER_PALETTE_TILE_LIGHT : RENDER_PALETTE_TILE_DARK;
			draw_rect_clipped(view, RENDER_TARGET_BOARD, rect, palette, 1, clip);
		}
	}
}

static void draw_zombie_fallback_clipped(RenderView *view, ZombieType type, IntRect rect, const IntRect *clip) {
	const int head = rect.h / 4 > 0 ? rect.h / 4 : 1;
	const int body_w = rect.w / 3 > 0 ? rect.w / 3 : 1;
	const int body_x = rect.x + rect.w / 2 - body_w / 2;

	draw_rect_clipped(view, RENDER_TARGET_BOARD, pvz_rect_make(body_x, rect.y + head, body_w, rect.h / 3),
					  RENDER_PALETTE_ZOMBIE, 0, clip);
	draw_rect_clipped(view, RENDER_TARGET_BOARD,
					  pvz_rect_make(body_x - body_w / 2, rect.y + head + rect.h / 12, body_w / 2, rect.h / 4),
					  RENDER_PALETTE_ZOMBIE, 0, clip);
	draw_rect_clipped(view, RENDER_TARGET_BOARD,
					  pvz_rect_make(body_x + body_w, rect.y + head + rect.h / 12, body_w / 2, rect.h / 4),
					  RENDER_PALETTE_ZOMBIE, 0, clip);
	draw_rect_clipped(view, RENDER_TARGET_BOARD,
					  pvz_rect_make(body_x, rect.y + head + rect.h / 3, body_w / 3, rect.h / 3), RENDER_PALETTE_ZOMBIE,
					  0, clip);
	draw_rect_clipped(view, RENDER_TARGET_BOARD,
					  pvz_rect_make(body_x + body_w * 2 / 3, rect.y + head + rect.h / 3, body_w / 3, rect.h / 3),
					  RENDER_PALETTE_ZOMBIE, 0, clip);
	draw_rect_clipped(view, RENDER_TARGET_BOARD, pvz_rect_make(body_x, rect.y + rect.h / 12, body_w, head),
					  RENDER_PALETTE_TEXT, 0, clip);

	if (type == ZOMBIE_CONE) {
		draw_rect_clipped(view, RENDER_TARGET_BOARD,
						  pvz_rect_make(body_x - body_w / 4, rect.y, body_w + body_w / 2, head), RENDER_PALETTE_CONE, 0,
						  clip);
	} else if (type == ZOMBIE_BUCKETHEAD) {
		draw_rect_clipped(view, RENDER_TARGET_BOARD,
						  pvz_rect_make(body_x - body_w / 5, rect.y, body_w + body_w / 3, head), RENDER_PALETTE_BUCKET,
						  0, clip);
	}
}

static const RenderSprite *select_zombie_sprite(const GameConfig *config, const Zombie *zombie) {
	const ZombieSpriteSet *sprite_set;
	const int max_health = pvz_zombie_max_health(config, zombie->type);
	const int max_armor = pvz_zombie_max_armor(config, zombie->type);
	const bool is_damaged = max_health > 0 && zombie->health <= max_health / 2;

	sprite_set = render_assets_get_zombie_sprites(zombie->type);
	if (!sprite_set) {
		return NULL;
	}

	if (max_armor > 0 && zombie->armor > 0 && sprite_set->armored) {
		return sprite_set->armored;
	}
	if (is_damaged && sprite_set->damaged) {
		return sprite_set->damaged;
	}
	if (sprite_set->base) {
		return sprite_set->base;
	}
	return sprite_set->damaged;
}

static void draw_zombie_clipped(RenderView *view, const GameConfig *config, const Zombie *zombie, IntRect rect,
								const IntRect *clip) {
	const RenderSprite *sprite = select_zombie_sprite(config, zombie);
	if (sprite) {
		draw_sprite_clipped(view, RENDER_TARGET_BOARD, sprite, rect.x, rect.y, clip);
		return;
	}

	draw_zombie_fallback_clipped(view, zombie->type, rect, clip);
}

static void draw_zombie(RenderView *view, const GameConfig *config, const Zombie *zombie, IntRect rect) {
	draw_zombie_clipped(view, config, zombie, rect, NULL);
}

static void draw_projectile_clipped(RenderView *view, IntRect rect, const IntRect *clip) {
	draw_rect_clipped(view, RENDER_TARGET_BOARD, rect, RENDER_PALETTE_PROJECTILE, 0, clip);
}

static void draw_projectile(RenderView *view, IntRect rect) { draw_projectile_clipped(view, rect, NULL); }

static void draw_sun_clipped(RenderView *view, RenderTarget target, IntRect rect, const IntRect *clip) {
	draw_rect_clipped(view, target, rect, RENDER_PALETTE_SUN, 0, clip);
	draw_rect_clipped(view, target, pvz_rect_make(rect.x + rect.w / 4, rect.y + rect.h / 4, rect.w / 2, rect.h / 2),
					  RENDER_PALETTE_HIGHLIGHT, 0, clip);
}

static void draw_sun(RenderView *view, RenderTarget target, IntRect rect) {
	draw_sun_clipped(view, target, rect, NULL);
}

static void capture_frame_data(FrameData *frame, const GameState *game, RenderStatus status) {
	GameWaveStatus wave_status;

	if (!frame || !game) {
		return;
	}

	memset(frame, 0, sizeof(*frame));
	game_get_wave_status(game, &wave_status);

	frame->selected_plant = game->selected_plant;
	memcpy(frame->seed_cooldowns, game->seed_cooldowns, sizeof(game->seed_cooldowns));
	frame->sun_count = game->sun_count;
	frame->paused = game->paused;
	frame->game_status = game->status;
	frame->level_progress_01 = wave_status.level_progress_01;
	frame->current_wave_index = wave_status.current_wave_index;
	frame->wave_count = wave_status.wave_count;
	frame->wave_warning_active = wave_status.warning_active;
	frame->current_wave_is_major = wave_status.current_wave_is_major;
	frame->flag_marker_count = wave_status.flag_marker_count;
	memcpy(frame->flag_marker_progress, wave_status.flag_marker_progress, sizeof(wave_status.flag_marker_progress));
	frame->status = status;
}

static void render_data_commit_baseline(RenderData *data) {
	if (!data) {
		return;
	}
	data->prev_frame = data->frame;
	data->prev_frame_valid = true;
}

static float get_seed_cooldown(const FrameData *frame, PlantType type) {
	switch (type) {
	case PLANT_SUNFLOWER:
		return frame->seed_cooldowns[0];
	case PLANT_PEASHOOTER:
		return frame->seed_cooldowns[1];
	case PLANT_WALLNUT:
		return frame->seed_cooldowns[2];
	case PLANT_NONE:
	default:
		return 0.0f;
	}
}

static float cooldown_ratio(const FrameData *frame, const GameConfig *config, PlantType type) {
	const int max_cooldown = pvz_plant_seed_cooldown(config, type);
	if (max_cooldown <= 0) {
		return 0.0f;
	}
	float ratio = get_seed_cooldown(frame, type) / (float)max_cooldown;
	if (ratio < 0.0f) {
		ratio = 0.0f;
	}
	if (ratio > 1.0f) {
		ratio = 1.0f;
	}
	return ratio;
}

static int cooldown_fill_pixels(IntRect rect, const FrameData *frame, const GameConfig *config, PlantType type) {
	const float ratio = cooldown_ratio(frame, config, type);
	int pixels = (int)ceilf(ratio * (float)rect.h);
	if (pixels < 0) {
		pixels = 0;
	}
	if (pixels > rect.h) {
		pixels = rect.h;
	}
	return pixels;
}

static IntRect cooldown_filled_rect(IntRect rect, int fill_pixels) {
	fill_pixels = clamp_int(fill_pixels, 0, rect.h);
	return pvz_rect_make(rect.x, rect.y + rect.h - fill_pixels, rect.w, fill_pixels);
}

static RenderPalette cooldown_empty_palette(void) {
	// The bar returns to this dark track color whenever cooldown pixels become empty.
	return RENDER_PALETTE_TILE_DARK;
}

static int wave_fill_width(const RenderView *view, const FrameData *frame) {
	const IntRect inner = wave_bar_inner_rect(view);
	int width = (int)lroundf((float)inner.w * frame->level_progress_01);
	if (width < 0) {
		width = 0;
	}
	if (width > inner.w) {
		width = inner.w;
	}
	return width;
}

static IntRect wave_fill_rect(const RenderView *view, int fill_width) {
	const IntRect inner = wave_bar_inner_rect(view);
	return pvz_rect_make(inner.x, inner.y, fill_width, inner.h);
}

static RenderPalette wave_fill_palette(const FrameData *frame) {
	return frame->wave_warning_active ? RENDER_PALETTE_WARNING : RENDER_PALETTE_SUCCESS;
}

static void format_wave_label(char *buffer, size_t size, const FrameData *frame) {
	const int current_wave = frame->wave_count > 0 ? frame->current_wave_index + 1 : 0;
	snprintf(buffer, size, "WAVE %d of %d", current_wave, frame->wave_count);
}

static void draw_wave_panel_base(RenderView *view) {
	const IntRect panel = wave_panel_rect(view);
	const IntRect bar = wave_bar_outer_rect(view);

	draw_rect(view, RENDER_TARGET_HUD, panel, RENDER_PALETTE_PANEL, 0);
	draw_rect(view, RENDER_TARGET_HUD, panel, RENDER_PALETTE_TEXT, 2);
	draw_rect(view, RENDER_TARGET_HUD, bar, RENDER_PALETTE_BG, 0);
	draw_rect(view, RENDER_TARGET_HUD, bar, RENDER_PALETTE_TEXT, 1);
}

static void draw_wave_markers(RenderView *view, const FrameData *frame, const IntRect *clip) {
	const IntRect bar = wave_bar_outer_rect(view);
	for (int i = 0; i < frame->flag_marker_count; ++i) {
		const int marker_x = bar.x + 1 + (frame->flag_marker_progress[i] * (bar.w - 2)) / 255;
		draw_rect_clipped(view, RENDER_TARGET_HUD, pvz_rect_make(marker_x, bar.y - 3, 2, bar.h + 6),
						  RENDER_PALETTE_WARNING, 0, clip);
		draw_rect_clipped(view, RENDER_TARGET_HUD, pvz_rect_make(marker_x, bar.y - 3, 8, 6), RENDER_PALETTE_HIGHLIGHT,
						  0, clip);
	}
}

static void draw_wave_label(RenderView *view, const FrameData *frame) {
	char buffer[24];
	const IntRect panel = wave_panel_rect(view);
	format_wave_label(buffer, sizeof(buffer), frame);
	draw_text_3x5(view, RENDER_TARGET_HUD, buffer, panel.x + 10, panel.y + 6, 2, RENDER_PALETTE_TEXT);
}

static void draw_wave_fill(RenderView *view, const FrameData *frame, const IntRect *clip) {
	const IntRect fill = wave_fill_rect(view, wave_fill_width(view, frame));
	if (fill.w <= 0) {
		return;
	}
	draw_rect_clipped(view, RENDER_TARGET_HUD, fill, wave_fill_palette(frame), 0, clip);
}

static void draw_wave_warning(RenderView *view, const FrameData *frame) {
	const IntRect warning = wave_warning_rect(view);
	if (!frame->wave_warning_active) {
		return;
	}

	draw_rect(view, RENDER_TARGET_HUD, warning, RENDER_PALETTE_WARNING, 0);
	draw_rect(view, RENDER_TARGET_HUD, warning, RENDER_PALETTE_TEXT, 1);
	draw_text_3x5(view, RENDER_TARGET_HUD, "HUGE WAVE INCOMING", warning.x + 10, warning.y + 4, 2,
				  RENDER_PALETTE_PANEL);
}

static void draw_card(RenderView *view, const RenderData *data, const GameConfig *config, int index, PlantType type) {
	char buffer[16];
	const RenderSprite *plant_sprite = render_assets_get_plant_sprite(type);
	const IntRect rect = determine_card_position(view, index);
	const bool selected = data->frame.selected_plant == type;
	const RenderPalette fill = selected ? RENDER_PALETTE_TILE_DARK : RENDER_PALETTE_TILE_DARK;
	const RenderPalette outline = RENDER_PALETTE_TEXT;

	draw_rect(view, RENDER_TARGET_HUD, rect, fill, 0);
	draw_rect(view, RENDER_TARGET_HUD, rect, outline, 2);

	const int icon_margin = 15;
	const int icon_size = rect.w - icon_margin * 2;
	const IntRect icon_rect = pvz_rect_make(rect.x + icon_margin, rect.y + 16, icon_size, icon_size);
	draw_sprite_scaled(view, RENDER_TARGET_HUD, plant_sprite, icon_rect);

	snprintf(buffer, sizeof(buffer), "%d", index + 1);
	draw_text_3x5(view, RENDER_TARGET_HUD, buffer, rect.x + rect.w - 20, rect.y + 12, 2, outline);

	const IntRect cost_panel = pvz_rect_make(rect.x + 5, rect.y + rect.h - 45, rect.w - 10, 40);
	snprintf(buffer, sizeof(buffer), "%d", pvz_plant_cost(config, type));
	draw_rect(view, RENDER_TARGET_HUD, cost_panel, RENDER_PALETTE_PANEL, 0);
	draw_rect(view, RENDER_TARGET_HUD, cost_panel, RENDER_PALETTE_TEXT, 2);
	draw_sun(view, RENDER_TARGET_HUD, pvz_rect_make(cost_panel.x + 4, cost_panel.y + 4, 32, 32));
	draw_text_3x5(view, RENDER_TARGET_HUD, buffer, cost_panel.x + 50, cost_panel.y + cost_panel.h - 24, 3,
				  RENDER_PALETTE_SUN);
}

static void draw_card_cooldown_full(RenderView *view, const FrameData *frame, const GameConfig *config, int index,
									PlantType type) {
	const IntRect rect = cooldown_bar_rect(view, index);
	const IntRect filled = cooldown_filled_rect(rect, cooldown_fill_pixels(rect, frame, config, type));

	draw_rect(view, RENDER_TARGET_HUD, rect, cooldown_empty_palette(), 0);
	if (!pvz_rect_is_empty(filled)) {
		draw_rect(view, RENDER_TARGET_HUD, filled, RENDER_PALETTE_BG, 0);
	}
}

static void update_text_element(RenderView *view, const char *previous, const char *current, int x, int y, int scale,
								RenderPalette background, RenderPalette foreground) {
	const IntRect prev_rect = text_rect_3x5(previous, x, y, scale);
	const IntRect cur_rect = text_rect_3x5(current, x, y, scale);
	const IntRect dirty = pvz_rect_union(prev_rect, cur_rect);

	if (pvz_rect_is_empty(dirty)) {
		return;
	}

	// Text is treated as a full-element redraw: clear the whole old/new union first, then redraw once.
	draw_rect(view, RENDER_TARGET_HUD, dirty, background, 0);
	draw_text_3x5(view, RENDER_TARGET_HUD, current, x, y, scale, foreground);
	mark_dirty_rect(view, RENDER_TARGET_HUD, dirty);
}

static void draw_board_entities(RenderView *view, const GameState *game, const IntRect *clip, int plant_padding,
								int zombie_size, int projectile_size, int sun_size) {
	for (int i = 0; i < PVZ_MAX_PLANTS; ++i) {
		if (!game->plants[i].active) {
			continue;
		}
		const Plant *plant = &game->plants[i];
		const RenderSprite *sprite = render_assets_get_plant_sprite(plant->type);
		const IntRect rect = board_cell_rect(game, plant->coord.row, plant->coord.col, plant_padding);
		draw_sprite_clipped(view, RENDER_TARGET_BOARD, sprite, rect.x, rect.y - 1, clip);
	}

	for (int i = 0; i < PVZ_MAX_ZOMBIES; ++i) {
		if (!game->zombies[i].active) {
			continue;
		}
		const Zombie *zombie = &game->zombies[i];
		draw_zombie_clipped(view, game->config, zombie,
							board_entity_rect(game, (float)zombie->lane + 0.5f, zombie->x + 0.5f, zombie_size), clip);
	}

	for (int i = 0; i < PVZ_MAX_PROJECTILES; ++i) {
		if (!game->projectiles[i].active) {
			continue;
		}
		draw_projectile_clipped(view,
								board_entity_rect(game, (float)game->projectiles[i].lane + 0.5f,
												  game->projectiles[i].x + 0.1f, projectile_size),
								clip);
	}

	for (int i = 0; i < PVZ_MAX_SUNS; ++i) {
		if (!game->suns[i].active) {
			continue;
		}
		draw_sun_clipped(view, RENDER_TARGET_BOARD, board_entity_rect(game, game->suns[i].y, game->suns[i].x, sun_size),
						 clip);
	}
}

static void draw_play_board_full(RenderView *view, const GameState *game, int plant_padding, int zombie_size,
								 int projectile_size, int sun_size) {
	clear_target(view, RENDER_TARGET_BOARD, RENDER_PALETTE_BLACK);
	draw_tile_checkerboard(view, game, NULL);
	draw_board_entities(view, game, NULL, plant_padding, zombie_size, projectile_size, sun_size);
}

static void draw_play_hud_static(RenderView *view, const RenderData *data, const GameState *game) {
	clear_target(view, RENDER_TARGET_HUD, RENDER_PALETTE_PANEL);
	draw_text_3x5(view, RENDER_TARGET_HUD, "SUN:", 10, 10, 3, RENDER_PALETTE_TEXT);
	draw_wave_panel_base(view);
	draw_card(view, data, game->config, 0, PLANT_SUNFLOWER);
	draw_card(view, data, game->config, 1, PLANT_PEASHOOTER);
	draw_card(view, data, game->config, 2, PLANT_WALLNUT);
}

static void draw_play_hud_dynamic(RenderView *view, const RenderData *data, const GameState *game) {
	char buffer[24];
	const IntRect sun_origin = sun_value_origin_rect(view);

	snprintf(buffer, sizeof(buffer), "%d", data->frame.sun_count);
	draw_text_3x5(view, RENDER_TARGET_HUD, buffer, sun_origin.x, sun_origin.y, 3, RENDER_PALETTE_TEXT);

	draw_wave_fill(view, &data->frame, NULL);
	draw_wave_markers(view, &data->frame, NULL);
	draw_wave_label(view, &data->frame);
	draw_wave_warning(view, &data->frame);

	draw_card_cooldown_full(view, &data->frame, game->config, 0, PLANT_SUNFLOWER);
	draw_card_cooldown_full(view, &data->frame, game->config, 1, PLANT_PEASHOOTER);
	draw_card_cooldown_full(view, &data->frame, game->config, 2, PLANT_WALLNUT);
}

void render_view_init(RenderView *view, int board_width, int board_height, int hud_width, int hud_height) {
	if (!view) {
		return;
	}

	memset(view, 0, sizeof(*view));
	view->board_width = clamp_int(board_width, 1, PVZ_MAX_BOARD_WIDTH);
	view->board_height = clamp_int(board_height, 1, PVZ_MAX_BOARD_HEIGHT);
	view->hud_width = clamp_int(hud_width, 1, PVZ_MAX_HUD_WIDTH);
	view->hud_height = clamp_int(hud_height, 1, PVZ_MAX_HUD_HEIGHT);
}

void render_view_begin(RenderView *view) {
	dirty_rect_list_clear(&view->board_dirty_rects);
	dirty_rect_list_clear(&view->hud_dirty_rects);
}

void render_data_init(RenderData *data) {
	if (!data) {
		return;
	}
	memset(data, 0, sizeof(*data));
	data->prev_frame_valid = false;
}

void render_data_update(RenderData *data, const GameState *game, RenderStatus status) {
	if (!data || !game) {
		return;
	}

	if (!data->prev_frame_valid) {
		capture_frame_data(&data->frame, game, status);
		render_data_commit_baseline(data);
		return;
	}

	data->prev_frame = data->frame;
	capture_frame_data(&data->frame, game, status);
}

uint16_t presentation_palette_to_rgb565(RenderPalette palette) {
	const PaletteRgb color = palette_rgb[palette];
	return (uint16_t)(((uint16_t)(color.r & 0xF8u) << 8) | ((uint16_t)(color.g & 0xFCu) << 3) |
					  ((uint16_t)color.b >> 3));
}

void dirty_rect_list_clear(DirtyRectList *rects) {
	if (!rects) {
		return;
	}
	rects->count = 0;
}

void presentation_prerender_play_view(RenderView *view, RenderData *data, const GameState *game) {
	const int unit_size = board_unit_size(game->config);
	const int plant_padding = unit_size / 8 > 0 ? unit_size / 8 : 1;
	const int zombie_size = 8;
	const int projectile_size = unit_size / 5 > 0 ? unit_size / 5 : 1;
	const int sun_size = unit_size / 3 > 0 ? unit_size / 3 : 1;

	render_data_update(data, game, RENDER_STATUS_NONE);

	draw_play_board_full(view, game, plant_padding, zombie_size, projectile_size, sun_size);
	draw_play_hud_static(view, data, game);
	draw_play_hud_dynamic(view, data, game);

	// Prerender seeds the retained framebuffers from scratch, so the first upload must cover both full targets.
	mark_full_target_dirty(view, RENDER_TARGET_BOARD);
	mark_full_target_dirty(view, RENDER_TARGET_HUD);

	render_data_commit_baseline(data);
}

void presentation_render_play_view(RenderView *view, RenderData *data, const GameState *game, RenderStatus status) {
	const int unit_size = board_unit_size(game->config);
	const int plant_padding = unit_size / 8 > 0 ? unit_size / 8 : 1;
	const int zombie_size = 8;
	const int projectile_size = unit_size / 5 > 0 ? unit_size / 5 : 1;
	const int sun_size = unit_size / 3 > 0 ? unit_size / 3 : 1;
	char previous_text[24];
	char current_text[24];
	const IntRect sun_origin = sun_value_origin_rect(view);
	int old_wave_fill;
	int new_wave_fill;

	render_data_update(data, game, status);
	old_wave_fill = wave_fill_width(view, &data->prev_frame);
	new_wave_fill = wave_fill_width(view, &data->frame);

	draw_play_board_full(view, game, plant_padding, zombie_size, projectile_size, sun_size);
	// Board dirty tracking is intentionally disabled; the entire board is refreshed as one region.
	mark_full_target_dirty(view, RENDER_TARGET_BOARD);

	snprintf(previous_text, sizeof(previous_text), "%d", data->prev_frame.sun_count);
	snprintf(current_text, sizeof(current_text), "%d", data->frame.sun_count);
	if (strcmp(previous_text, current_text) != 0) {
		update_text_element(view, previous_text, current_text, sun_origin.x, sun_origin.y, 3, RENDER_PALETTE_PANEL,
							RENDER_PALETTE_TEXT);
	}

	format_wave_label(previous_text, sizeof(previous_text), &data->prev_frame);
	format_wave_label(current_text, sizeof(current_text), &data->frame);
	if (strcmp(previous_text, current_text) != 0) {
		const IntRect panel = wave_panel_rect(view);
		update_text_element(view, previous_text, current_text, panel.x + 10, panel.y + 6, 2, RENDER_PALETTE_PANEL,
							RENDER_PALETTE_TEXT);
	}

	if (wave_fill_palette(&data->prev_frame) != wave_fill_palette(&data->frame) || old_wave_fill != new_wave_fill) {
		if (wave_fill_palette(&data->prev_frame) != wave_fill_palette(&data->frame)) {
			const IntRect dirty = wave_fill_rect(view, old_wave_fill > new_wave_fill ? old_wave_fill : new_wave_fill);
			if (!pvz_rect_is_empty(dirty)) {
				// A color swap changes every filled pixel in the overlap, so redraw the whole filled span union.
				draw_rect(view, RENDER_TARGET_HUD, dirty, RENDER_PALETTE_BG, 0);
				draw_wave_fill(view, &data->frame, &dirty);
				draw_wave_markers(view, &data->frame, &dirty);
				mark_dirty_rect(view, RENDER_TARGET_HUD, dirty);
			}
		} else if (new_wave_fill > old_wave_fill) {
			const IntRect dirty =
				pvz_rect_make(wave_bar_inner_rect(view).x + old_wave_fill, wave_bar_inner_rect(view).y,
							  new_wave_fill - old_wave_fill, wave_bar_inner_rect(view).h);
			if (!pvz_rect_is_empty(dirty)) {
				// Progress growth dirties only the newly-filled horizontal strip.
				draw_rect(view, RENDER_TARGET_HUD, dirty, wave_fill_palette(&data->frame), 0);
				draw_wave_markers(view, &data->frame, &dirty);
				mark_dirty_rect(view, RENDER_TARGET_HUD, dirty);
			}
		} else {
			const IntRect dirty =
				pvz_rect_make(wave_bar_inner_rect(view).x + new_wave_fill, wave_bar_inner_rect(view).y,
							  old_wave_fill - new_wave_fill, wave_bar_inner_rect(view).h);
			if (!pvz_rect_is_empty(dirty)) {
				// Progress shrink clears only the trailing strip that is no longer filled.
				draw_rect(view, RENDER_TARGET_HUD, dirty, RENDER_PALETTE_BG, 0);
				draw_wave_markers(view, &data->frame, &dirty);
				mark_dirty_rect(view, RENDER_TARGET_HUD, dirty);
			}
		}
	}

	if (data->prev_frame.wave_warning_active != data->frame.wave_warning_active) {
		const IntRect warning = wave_warning_rect(view);
		// The warning banner is text-plus-box UI, so it follows the full-element clear/redraw rule.
		draw_rect(view, RENDER_TARGET_HUD, warning, RENDER_PALETTE_PANEL, 0);
		draw_wave_warning(view, &data->frame);
		mark_dirty_rect(view, RENDER_TARGET_HUD, warning);
	}

	for (int index = 0; index < 3; ++index) {
		const PlantType type = (index == 0) ? PLANT_SUNFLOWER : (index == 1) ? PLANT_PEASHOOTER : PLANT_WALLNUT;
		const IntRect rect = cooldown_bar_rect(view, index);
		const int old_fill = cooldown_fill_pixels(rect, &data->prev_frame, game->config, type);
		const int new_fill = cooldown_fill_pixels(rect, &data->frame, game->config, type);

		if (new_fill == old_fill) {
			continue;
		}

		if (new_fill > old_fill) {
			// Cooldown resets redraw the full bar because filled pixels can reappear anywhere inside the element.
			draw_card_cooldown_full(view, &data->frame, game->config, index, type);
			mark_dirty_rect(view, RENDER_TARGET_HUD, rect);
		} else {
			const IntRect dirty = pvz_rect_make(rect.x, rect.y + rect.h - old_fill, rect.w, old_fill - new_fill);
			if (!pvz_rect_is_empty(dirty)) {
				// Cooldown decay clears only the exact strip that transitioned from filled to empty.
				draw_rect(view, RENDER_TARGET_HUD, dirty, cooldown_empty_palette(), 0);
				mark_dirty_rect(view, RENDER_TARGET_HUD, dirty);
			}
		}
	}
}

void presentation_render_placeholder_view(RenderView *view, const GameConfig *config) {
	(void)config;

	clear_target(view, RENDER_TARGET_BOARD, RENDER_PALETTE_BLACK);
	clear_target(view, RENDER_TARGET_HUD, RENDER_PALETTE_BG);

	for (int y = 0; y < view->board_height; ++y) {
		for (int x = 0; x < view->board_width; ++x) {
			if (((x / 8) + (y / 8)) % 2 == 0) {
				set_pixel(view, RENDER_TARGET_BOARD, x, y, RENDER_PALETTE_TILE_DARK);
			} else {
				set_pixel(view, RENDER_TARGET_BOARD, x, y, RENDER_PALETTE_TILE_LIGHT);
			}
		}
	}

	draw_rect(
		view, RENDER_TARGET_BOARD,
		pvz_rect_make(view->board_width / 4, view->board_height / 4, view->board_width / 2, view->board_height / 2),
		RENDER_PALETTE_PANEL, 0);
	draw_rect(view, RENDER_TARGET_BOARD,
			  pvz_rect_make(view->board_width / 4 + 4, view->board_height / 4 + 4, view->board_width / 2 - 8,
							view->board_height / 2 - 8),
			  RENDER_PALETTE_BLACK, 0);

	mark_full_target_dirty(view, RENDER_TARGET_BOARD);
	mark_full_target_dirty(view, RENDER_TARGET_HUD);
}
