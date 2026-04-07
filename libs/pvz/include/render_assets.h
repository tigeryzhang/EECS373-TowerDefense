#pragma once

#include <stdint.h>

#include "game_types.h"

#define SPRITE_PIXEL_TRANSPARENT 0xFFu

typedef struct {
	uint8_t width;
	uint8_t height;
	const uint8_t *pixels;
} RenderSprite;

typedef struct {
	const RenderSprite *armored;
	const RenderSprite *base;
	const RenderSprite *damaged;
} ZombieSpriteSet;

// Sprite data is row-major and each entry is either a RenderPalette index or
// SPRITE_PIXEL_TRANSPARENT.
const RenderSprite *render_assets_get_plant_sprite(PlantType type);
const ZombieSpriteSet *render_assets_get_zombie_sprites(ZombieType type);
