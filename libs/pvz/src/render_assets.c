#include "render_assets.h"

#include <stddef.h>

#include "presentation.h"

#define T SPRITE_PIXEL_TRANSPARENT
#define X RENDER_PALETTE_TEXT
#define K RENDER_PALETTE_BUCKET
#define N RENDER_PALETTE_SUN
#define L RENDER_PALETTE_WALLNUT
#define P RENDER_PALETTE_PLANT

#define A RENDER_PALETTE_ART_0
#define B RENDER_PALETTE_ART_1
#define C RENDER_PALETTE_ART_2
#define D RENDER_PALETTE_ART_3
#define E RENDER_PALETTE_ART_4
#define F RENDER_PALETTE_ART_5
#define G RENDER_PALETTE_ART_6
#define H RENDER_PALETTE_ART_7
#define I RENDER_PALETTE_ART_8

// clang-format off
static const uint8_t zombie_regular_base_pixels[8 * 8] = {
	T, T, T, T, T, T, T, T,
	T, T, B, B, B, B, T, T,
	T, B, A, A, A, A, B, T,
	T, B, X, A, X, A, B, T,
	T, B, A, A, A, A, B, T,
	T, T, B, C, C, B, T, T,
	T, T, C, D, C, T, T, T,
	T, T, T, D, D, T, T, T,
};

static const uint8_t zombie_regular_damaged_pixels[8 * 8] = {
    T, T, T, T, T, T, T, T,
	T, T, B, B, B, B, T, T,
	T, B, A, A, A, A, B, T,
	T, B, E, A, X, A, B, T,
	T, B, A, A, A, A, B, T,
	T, T, B, C, C, E, T, T,
	T, T, E, D, C, T, T, T,
	T, T, T, D, D, T, T, T,
};

static const uint8_t zombie_cone_armored_pixels[8 * 8] = {
    T, T, T, G, G, T, T, T,
	T, T, G, G, G, G, T, T,
	T, B, A, A, A, A, B, T,
	T, B, X, A, X, A, B, T,
	T, B, A, A, A, A, B, T,
	T, T, B, C, C, B, T, T,
	T, T, C, D, C, T, T, T,
	T, T, T, D, D, T, T, T,
};

static const uint8_t zombie_bucket_armored_pixels[8 * 8] = {
    T, T, K, K, K, T, T, T,
	T, T, K, K, K, K, T, T,
	T, B, A, A, A, A, B, T,
	T, B, X, A, X, A, B, T,
	T, B, A, A, A, A, B, T,
	T, T, B, C, C, B, T, T,
	T, T, C, D, C, T, T, T,
	T, T, T, D, D, T, T, T,
};

static const uint8_t plant_sunflower_pixels[8 * 8] = {
	T, T, T, N, T, N, T, T,
	N, T, N, G, N, G, N, T,
	T, N, L, L, L, L, G, N,
	N, G, L, X, X, L, G, N,
	T, N, G, L, L, G, N, T,
	T, T, N, P, P, N, T, N,
	T, P, T, P, T, P, T, T,
	T, T, P, P, P, T, T, T,
};

static const uint8_t plant_peashooter_pixels[8 * 8] = {
	T, T, H, H, T, T, T, T,
	T, H, F, F, F, T, T, T,
	P, F, F, X, F, H, F, T,
	T, F, F, F, F, F, P, T,
	T, T, F, P, P, T, T, T,
	T, T, P, T, T, T, T, T,
	P, T, P, T, F, T, T, T,
	T, P, P, P, T, T, T, T,
};

static const uint8_t plant_wallnut_pixels[8 * 8] = {
	T, T, T, T, T, T, T, T,
	T, T, T, I, I, T, T, T,
	T, T, I, I,	I, I, T, T,
	T, C, I, X,	I, X, I, T,
	T, L, I, I, I, I, I, T,
	T, L, I, I, C, C, I, T,
	T, L, L, I, I, I, C, T,
	T, T, L, L, L, L, T, T,
};
// clang-format on

static const RenderSprite zombie_regular_base_sprite = {8, 8, zombie_regular_base_pixels};
static const RenderSprite zombie_regular_damaged_sprite = {8, 8, zombie_regular_damaged_pixels};
static const RenderSprite zombie_cone_armored_sprite = {8, 8, zombie_cone_armored_pixels};
static const RenderSprite zombie_bucket_armored_sprite = {8, 8, zombie_bucket_armored_pixels};

static const RenderSprite plant_sunflower_sprite = {8, 8, plant_sunflower_pixels};
static const RenderSprite plant_peashooter_sprite = {8, 8, plant_peashooter_pixels};
static const RenderSprite plant_wallnut_sprite = {8, 8, plant_wallnut_pixels};

static const ZombieSpriteSet zombie_regular_sprites = {
	.armored = &zombie_regular_base_sprite,
	.base = &zombie_regular_base_sprite,
	.damaged = &zombie_regular_damaged_sprite,
};

static const ZombieSpriteSet zombie_cone_sprites = {
	.armored = &zombie_cone_armored_sprite,
	.base = &zombie_regular_base_sprite,
	.damaged = &zombie_regular_damaged_sprite,
};

static const ZombieSpriteSet zombie_bucket_sprites = {
	.armored = &zombie_bucket_armored_sprite,
	.base = &zombie_regular_base_sprite,
	.damaged = &zombie_regular_damaged_sprite,
};

const RenderSprite *render_assets_get_plant_sprite(PlantType type) {
	switch (type) {
	case PLANT_SUNFLOWER:
		return &plant_sunflower_sprite;
	case PLANT_PEASHOOTER:
		return &plant_peashooter_sprite;
	case PLANT_WALLNUT:
		return &plant_wallnut_sprite;
	case PLANT_NONE:
	default:
		return NULL;
	}
}

const ZombieSpriteSet *render_assets_get_zombie_sprites(ZombieType type) {
	switch (type) {
	case ZOMBIE_REGULAR:
		return &zombie_regular_sprites;
	case ZOMBIE_CONE:
		return &zombie_cone_sprites;
	case ZOMBIE_BUCKETHEAD:
		return &zombie_bucket_sprites;
	case ZOMBIE_NONE:
	default:
		return NULL;
	}
}

#undef T
#undef X
#undef K
#undef N
#undef L
#undef P

#undef A
#undef B
#undef C
#undef D
#undef E
#undef F
#undef G
#undef H
#undef I
