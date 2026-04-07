#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "game_types.h"
#include "pvz_config.h"

#define PVZ_LANE_AUTO (-1)
#define PVZ_LANE_MASK_ALL 0xFFu

#define PVZ_WAVE_FLAG_NONE 0u
#define PVZ_WAVE_FLAG_MAJOR 0x01u

#define PVZ_MAX_WAVE_FLAG_MARKERS 8

typedef struct {
	ZombieType type;
	uint8_t count;
	float first_spawn_delay_sec;
	float spawn_interval_sec;
	int8_t lane;
	uint8_t lane_mask;
} PvzSpawnGroup;

typedef struct {
	const PvzSpawnGroup *groups;
	uint8_t group_count;
	float start_delay_sec;
	uint8_t drain_threshold;
	uint8_t flags;
	uint8_t weight;
} PvzWaveDef;

typedef struct {
	const PvzWaveDef *waves;
	uint8_t wave_count;
	float opening_delay_sec;
	int16_t starting_sun_override;
} PvzLevelDef;

typedef struct {
	float level_progress_01;
	uint8_t current_wave_index;
	uint8_t wave_count;
	bool warning_active;
	bool current_wave_is_major;
	uint8_t flag_marker_count;
	uint8_t flag_marker_progress[PVZ_MAX_WAVE_FLAG_MARKERS];
} GameWaveStatus;

typedef struct {
	uint8_t level_index;
	uint8_t wave_index;
	uint8_t group_index;
	uint8_t spawns_left_in_group;
	uint8_t next_auto_lane;
	bool level_exhausted;
	bool wave_started;
	bool warning_active;
	uint16_t ticks_until_wave;
	uint16_t ticks_until_spawn;
	uint16_t warning_ticks_remaining;
	uint16_t completed_wave_weight;
	uint16_t level_total_weight;
	uint16_t current_wave_spawns_spawned;
	uint16_t current_wave_spawns_total;
} GameWaveRuntime;

typedef struct {
	bool active;
	PlantType type;
	BoardCoord coord;
	int health;
	float action_timer;
} Plant;

typedef struct {
	bool active;
	ZombieType type;
	int lane;
	float x;
	int health;
	int armor;
	float speed;
	float attack_timer;
} Zombie;

typedef struct {
	bool active;
	int lane;
	float x;
	int damage;
	float speed;
} Projectile;

typedef struct {
	bool active;
	float x;
	float y;
	int value;
	float collect_timer;
} Sun;

// TODO: Add Lawnmowers
typedef struct {
	const GameConfig *config;
	int sun_count;
	PlantType selected_plant;
	float seed_cooldowns[PVZ_NUM_PLANT_TYPES];
	GameStatus status;
	bool paused;
	GameCommandResult last_command_result;
	GameWaveRuntime wave_runtime;
	int plant_grid[PVZ_MAX_ROWS][PVZ_MAX_COLS];
	Plant plants[PVZ_MAX_PLANTS];
	Zombie zombies[PVZ_MAX_ZOMBIES];
	Projectile projectiles[PVZ_MAX_PROJECTILES];
	Sun suns[PVZ_MAX_SUNS];
} GameState;

void game_init(GameState *state, const GameConfig *config);
void game_reset(GameState *state);
void game_set_level(GameState *state, uint8_t level_index);
uint8_t game_get_level_count(void);
void game_get_wave_status(const GameState *state, GameWaveStatus *status);
GameCommandResult game_apply_command(GameState *state, GameCommand command);
void game_step(GameState *state, float delta);
bool game_coord_in_bounds(const GameState *state, BoardCoord coord);
bool game_has_active_zombies(const GameState *state);
