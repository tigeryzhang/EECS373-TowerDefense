// Needs the pvz prefix to avoid naming conflicts
#pragma once

#include "game_types.h"

#include <stdbool.h>

#define PVZ_MAX_ROWS 8
#define PVZ_MAX_COLS 12

#define PVZ_MAX_PLANTS 48
#define PVZ_MAX_ZOMBIES 32
#define PVZ_MAX_PROJECTILES 64
#define PVZ_MAX_SUNS 32

#define PVZ_MAX_BOARD_WIDTH 64
#define PVZ_MAX_BOARD_HEIGHT 32
#define PVZ_MAX_BOARD_PIXELS (PVZ_MAX_BOARD_WIDTH * PVZ_MAX_BOARD_HEIGHT)

#define PVZ_MAX_HUD_WIDTH 480
#define PVZ_MAX_HUD_HEIGHT 320
#define PVZ_MAX_HUD_PIXELS (PVZ_MAX_HUD_WIDTH * PVZ_MAX_HUD_HEIGHT)

#define PVZ_MAX_INPUT_COMMANDS 32
#define PVZ_DEFAULT_TARGET_FPS 20.0f
#define PVZ_DEFAULT_FIXED_DT (1.0f / PVZ_DEFAULT_TARGET_FPS)

typedef struct {
	// Board layout
	int rows;
	int cols;
	int tile_size;

	// Render target resolutions
	int board_x_resolution;
	int board_y_resolution;
	int hud_x_resolution;
	int hud_y_resolution;

	// Game setup
	int starting_sun;
	bool start_with_demo_layout;

	// Seed packets
	int sunflower_cost;
	int sunflower_seed_cooldown;
	int peashooter_cost;
	int peashooter_seed_cooldown;
	int wallnut_cost;
	int wallnut_seed_cooldown;

	// Plant stats
	int sunflower_health;
	int peashooter_health;
	int wallnut_health;
	int pea_damage;

	// Zombie stats
	int zombie_health;
	int cone_health;
	int bucket_health;
	int cone_armor;
	int bucket_armor;
	int zombie_bite_damage;

	// Timing
	float fixed_dt;
	float zombie_spawn_interval;
	float zombie_attack_interval;
	float sunflower_sun_interval;
	float sunflower_collect_delay;
	float peashooter_fire_interval;

	// Movement
	float pea_speed_cells_per_second;
	float zombie_speed_regular;
	float zombie_speed_cone;
	float zombie_speed_bucket;
} GameConfig;

GameConfig pvz_make_default_config(void);
void pvz_clamp_config(GameConfig *config);

int pvz_plant_cost(const GameConfig *config, PlantType type);
int pvz_plant_seed_cooldown(const GameConfig *config, PlantType type);
int pvz_zombie_max_health(const GameConfig *config, ZombieType type);
int pvz_zombie_max_armor(const GameConfig *config, ZombieType type);
