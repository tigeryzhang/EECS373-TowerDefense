#include "game.h"
#include "game_types.h"
#include "pvz_config.h"
#include "pvz_utils.h"

#include <math.h>
#include <string.h>

#define PVZ_MAJOR_WAVE_WARNING_SEC 2.0f

static const PvzSpawnGroup level_0_wave_0_groups[] = {
	{.type = ZOMBIE_REGULAR,
	 .count = 3,
	 .first_spawn_delay_sec = 0.0f,
	 .spawn_interval_sec = 1.8f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
};

static const PvzSpawnGroup level_0_wave_1_groups[] = {
	{.type = ZOMBIE_REGULAR,
	 .count = 4,
	 .first_spawn_delay_sec = 0.0f,
	 .spawn_interval_sec = 1.4f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
	{.type = ZOMBIE_CONE,
	 .count = 2,
	 .first_spawn_delay_sec = 1.3f,
	 .spawn_interval_sec = 2.0f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
};

static const PvzSpawnGroup level_0_wave_2_groups[] = {
	{.type = ZOMBIE_REGULAR,
	 .count = 3,
	 .first_spawn_delay_sec = 0.0f,
	 .spawn_interval_sec = 1.1f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
	{.type = ZOMBIE_CONE,
	 .count = 3,
	 .first_spawn_delay_sec = 0.8f,
	 .spawn_interval_sec = 1.4f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
	{.type = ZOMBIE_BUCKETHEAD,
	 .count = 1,
	 .first_spawn_delay_sec = 1.2f,
	 .spawn_interval_sec = 1.6f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
};

static const PvzWaveDef level_0_waves[] = {
	{.groups = level_0_wave_0_groups,
	 .group_count = 1,
	 .start_delay_sec = 0.0f,
	 .drain_threshold = 0,
	 .flags = PVZ_WAVE_FLAG_NONE,
	 .weight = 3},
	{.groups = level_0_wave_1_groups,
	 .group_count = 2,
	 .start_delay_sec = 3.0f,
	 .drain_threshold = 1,
	 .flags = PVZ_WAVE_FLAG_NONE,
	 .weight = 4},
	{.groups = level_0_wave_2_groups,
	 .group_count = 3,
	 .start_delay_sec = 4.0f,
	 .drain_threshold = 1,
	 .flags = PVZ_WAVE_FLAG_MAJOR,
	 .weight = 5},
};

static const PvzSpawnGroup level_1_wave_0_groups[] = {
	{.type = ZOMBIE_REGULAR,
	 .count = 4,
	 .first_spawn_delay_sec = 0.0f,
	 .spawn_interval_sec = 1.3f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = 0x0Fu},
};

static const PvzSpawnGroup level_1_wave_1_groups[] = {
	{.type = ZOMBIE_REGULAR,
	 .count = 5,
	 .first_spawn_delay_sec = 0.0f,
	 .spawn_interval_sec = 1.0f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
	{.type = ZOMBIE_CONE,
	 .count = 2,
	 .first_spawn_delay_sec = 1.0f,
	 .spawn_interval_sec = 1.6f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
};

static const PvzSpawnGroup level_1_wave_2_groups[] = {
	{.type = ZOMBIE_CONE,
	 .count = 4,
	 .first_spawn_delay_sec = 0.0f,
	 .spawn_interval_sec = 1.0f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
	{.type = ZOMBIE_REGULAR,
	 .count = 3,
	 .first_spawn_delay_sec = 0.7f,
	 .spawn_interval_sec = 1.2f,
	 .lane = 0,
	 .lane_mask = 0},
};

static const PvzSpawnGroup level_1_wave_3_groups[] = {
	{.type = ZOMBIE_REGULAR,
	 .count = 4,
	 .first_spawn_delay_sec = 0.0f,
	 .spawn_interval_sec = 0.9f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
	{.type = ZOMBIE_BUCKETHEAD,
	 .count = 2,
	 .first_spawn_delay_sec = 1.2f,
	 .spawn_interval_sec = 2.0f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = 0x0Au},
};

static const PvzSpawnGroup level_1_wave_4_groups[] = {
	{.type = ZOMBIE_CONE,
	 .count = 4,
	 .first_spawn_delay_sec = 0.0f,
	 .spawn_interval_sec = 0.8f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
	{.type = ZOMBIE_BUCKETHEAD,
	 .count = 3,
	 .first_spawn_delay_sec = 0.8f,
	 .spawn_interval_sec = 1.4f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
};

static const PvzWaveDef level_1_waves[] = {
	{.groups = level_1_wave_0_groups,
	 .group_count = 1,
	 .start_delay_sec = 0.0f,
	 .drain_threshold = 0,
	 .flags = PVZ_WAVE_FLAG_NONE,
	 .weight = 3},
	{.groups = level_1_wave_1_groups,
	 .group_count = 2,
	 .start_delay_sec = 2.5f,
	 .drain_threshold = 2,
	 .flags = PVZ_WAVE_FLAG_NONE,
	 .weight = 4},
	{.groups = level_1_wave_2_groups,
	 .group_count = 2,
	 .start_delay_sec = 3.0f,
	 .drain_threshold = 2,
	 .flags = PVZ_WAVE_FLAG_MAJOR,
	 .weight = 5},
	{.groups = level_1_wave_3_groups,
	 .group_count = 2,
	 .start_delay_sec = 2.5f,
	 .drain_threshold = 2,
	 .flags = PVZ_WAVE_FLAG_NONE,
	 .weight = 4},
	{.groups = level_1_wave_4_groups,
	 .group_count = 2,
	 .start_delay_sec = 3.0f,
	 .drain_threshold = 1,
	 .flags = PVZ_WAVE_FLAG_MAJOR,
	 .weight = 6},
};

static const PvzSpawnGroup level_2_wave_0_groups[] = {
	{.type = ZOMBIE_REGULAR,
	 .count = 5,
	 .first_spawn_delay_sec = 0.0f,
	 .spawn_interval_sec = 1.0f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
	{.type = ZOMBIE_CONE,
	 .count = 2,
	 .first_spawn_delay_sec = 1.4f,
	 .spawn_interval_sec = 1.8f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
};

static const PvzSpawnGroup level_2_wave_1_groups[] = {
	{.type = ZOMBIE_BUCKETHEAD,
	 .count = 3,
	 .first_spawn_delay_sec = 0.0f,
	 .spawn_interval_sec = 2.2f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = 0x15u},
	{.type = ZOMBIE_CONE,
	 .count = 3,
	 .first_spawn_delay_sec = 1.0f,
	 .spawn_interval_sec = 1.7f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
};

static const PvzSpawnGroup level_2_wave_2_groups[] = {
	{.type = ZOMBIE_BUCKETHEAD,
	 .count = 4,
	 .first_spawn_delay_sec = 0.0f,
	 .spawn_interval_sec = 1.9f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
};

static const PvzSpawnGroup level_2_wave_3_groups[] = {
	{.type = ZOMBIE_CONE,
	 .count = 4,
	 .first_spawn_delay_sec = 0.0f,
	 .spawn_interval_sec = 0.8f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
	{.type = ZOMBIE_BUCKETHEAD,
	 .count = 4,
	 .first_spawn_delay_sec = 0.8f,
	 .spawn_interval_sec = 1.1f,
	 .lane = PVZ_LANE_AUTO,
	 .lane_mask = PVZ_LANE_MASK_ALL},
};

static const PvzWaveDef level_2_waves[] = {
	{.groups = level_2_wave_0_groups,
	 .group_count = 2,
	 .start_delay_sec = 0.0f,
	 .drain_threshold = 0,
	 .flags = PVZ_WAVE_FLAG_NONE,
	 .weight = 4},
	{.groups = level_2_wave_1_groups,
	 .group_count = 2,
	 .start_delay_sec = 3.5f,
	 .drain_threshold = 2,
	 .flags = PVZ_WAVE_FLAG_MAJOR,
	 .weight = 5},
	{.groups = level_2_wave_2_groups,
	 .group_count = 1,
	 .start_delay_sec = 4.0f,
	 .drain_threshold = 1,
	 .flags = PVZ_WAVE_FLAG_NONE,
	 .weight = 5},
	{.groups = level_2_wave_3_groups,
	 .group_count = 2,
	 .start_delay_sec = 4.0f,
	 .drain_threshold = 1,
	 .flags = PVZ_WAVE_FLAG_MAJOR,
	 .weight = 7},
};

static const PvzLevelDef builtin_levels[] = {
	{.waves = level_0_waves, .wave_count = 3, .opening_delay_sec = 1.2f, .starting_sun_override = -1},
	{.waves = level_1_waves, .wave_count = 5, .opening_delay_sec = 1.0f, .starting_sun_override = -1},
	{.waves = level_2_waves, .wave_count = 4, .opening_delay_sec = 1.0f, .starting_sun_override = 225},
};

static uint8_t clamp_level_index(uint8_t level_index) {
	const uint8_t level_count = (uint8_t)(sizeof(builtin_levels) / sizeof(builtin_levels[0]));
	return level_index < level_count ? level_index : 0;
}

static const PvzLevelDef *current_level_def(const GameState *state) {
	return &builtin_levels[clamp_level_index(state->wave_runtime.level_index)];
}

static uint8_t current_wave_weight(const PvzWaveDef *wave) { return wave->weight > 0 ? wave->weight : 1; }

static uint16_t seconds_to_runtime_ticks(const GameState *state, float seconds) {
	const float fallback_dt = PVZ_DEFAULT_FIXED_DT;
	const float fixed_dt =
		(state && state->config && state->config->fixed_dt > 0.0f) ? state->config->fixed_dt : fallback_dt;

	if (seconds <= 0.0f) {
		return 0;
	}

	float runtime_ticks = seconds / fixed_dt;
	if (runtime_ticks < 1.0f) {
		runtime_ticks = 1.0f;
	}
	if (runtime_ticks > 65535.0f) {
		runtime_ticks = 65535.0f;
	}
	return (uint16_t)(runtime_ticks + 0.5f);
}

static void clear_state_arrays(GameState *state) {
	memset(state->plants, 0, sizeof(state->plants));
	memset(state->zombies, 0, sizeof(state->zombies));
	memset(state->projectiles, 0, sizeof(state->projectiles));
	memset(state->suns, 0, sizeof(state->suns));
	for (int row = 0; row < PVZ_MAX_ROWS; ++row) {
		for (int col = 0; col < PVZ_MAX_COLS; ++col) {
			state->plant_grid[row][col] = -1;
		}
	}
}

static int get_free_plant_slot(GameState *state) {
	for (int i = 0; i < PVZ_MAX_PLANTS; ++i) {
		if (!state->plants[i].active) {
			return i;
		}
	}
	return -1;
}

static int get_free_zombie_slot(GameState *state) {
	for (int i = 0; i < PVZ_MAX_ZOMBIES; ++i) {
		if (!state->zombies[i].active) {
			return i;
		}
	}
	return -1;
}

static int get_free_projectile_slot(GameState *state) {
	for (int i = 0; i < PVZ_MAX_PROJECTILES; ++i) {
		if (!state->projectiles[i].active) {
			return i;
		}
	}
	return -1;
}

static int get_free_sun_slot(GameState *state) {
	for (int i = 0; i < PVZ_MAX_SUNS; ++i) {
		if (!state->suns[i].active) {
			return i;
		}
	}
	return -1;
}

static int get_plant_health(const GameState *state, PlantType plant_type) {
	switch (plant_type) {
	case PLANT_SUNFLOWER:
		return state->config->sunflower_health;
	case PLANT_PEASHOOTER:
		return state->config->peashooter_health;
	case PLANT_WALLNUT:
		return state->config->wallnut_health;
	case PLANT_NONE:
	default:
		return 0;
	}
}

static float get_plant_seed_cooldown(const GameState *state, PlantType plant_type) {
	// TODO: Maybe change semantic coupling to indices so we can change out different packets
	switch (plant_type) {
	case PLANT_SUNFLOWER:
		return state->seed_cooldowns[0];
	case PLANT_PEASHOOTER:
		return state->seed_cooldowns[1];
	case PLANT_WALLNUT:
		return state->seed_cooldowns[2];
	case PLANT_NONE:
	default:
		return 0;
	}
}

static void activate_plant_seed_cooldown(GameState *state, PlantType plant_type) {
	switch (plant_type) {
	case PLANT_SUNFLOWER:
		state->seed_cooldowns[0] = pvz_plant_seed_cooldown(state->config, plant_type);
		break;
	case PLANT_PEASHOOTER:
		state->seed_cooldowns[1] = pvz_plant_seed_cooldown(state->config, plant_type);
		break;
	case PLANT_WALLNUT:
		state->seed_cooldowns[2] = pvz_plant_seed_cooldown(state->config, plant_type);
		break;
	case PLANT_NONE:
		break;
	}
}

static float get_zombie_speed(const GameState *state, ZombieType zombie_type) {
	switch (zombie_type) {
	case ZOMBIE_CONE:
		return state->config->zombie_speed_cone;
	case ZOMBIE_BUCKETHEAD:
		return state->config->zombie_speed_bucket;
	case ZOMBIE_REGULAR:
	default:
		return state->config->zombie_speed_regular;
	}
}

static int get_zombie_health(const GameState *state, ZombieType zombie_type) {
	switch (zombie_type) {
	case ZOMBIE_CONE:
		return state->config->cone_health;
	case ZOMBIE_BUCKETHEAD:
		return state->config->bucket_health;
	case ZOMBIE_REGULAR:
	default:
		return state->config->zombie_health;
	}
}

static int get_zombie_armor(const GameState *state, ZombieType zombie_type) {
	switch (zombie_type) {
	case ZOMBIE_CONE:
		return state->config->cone_armor;
	case ZOMBIE_BUCKETHEAD:
		return state->config->bucket_armor;
	case ZOMBIE_REGULAR:
	default:
		return 0;
	}
}

static int count_active_zombies(const GameState *state) {
	int count = 0;
	for (int i = 0; i < PVZ_MAX_ZOMBIES; ++i) {
		if (state->zombies[i].active) {
			++count;
		}
	}
	return count;
}

static uint8_t active_lane_mask(const GameState *state) {
	uint8_t mask = 0;
	const int rows = clamp_int(state->config->rows, 0, PVZ_MAX_ROWS);
	for (int row = 0; row < rows; ++row) {
		mask |= (uint8_t)(1u << row);
	}
	return mask;
}

static uint8_t sanitize_lane_mask(const GameState *state, uint8_t lane_mask) {
	const uint8_t valid_mask = active_lane_mask(state);
	uint8_t sanitized = lane_mask & valid_mask;
	if (sanitized == 0) {
		sanitized = valid_mask;
	}
	return sanitized;
}

static int select_auto_lane(GameState *state, uint8_t lane_mask) {
	// Deterministic lane selection

	const int rows = state->config->rows;
	const uint8_t valid_mask = sanitize_lane_mask(state, lane_mask);
	int lane_counts[PVZ_MAX_ROWS] = {0};

	for (int i = 0; i < PVZ_MAX_ZOMBIES; ++i) {
		if (!state->zombies[i].active) {
			continue;
		}
		if (state->zombies[i].lane >= 0 && state->zombies[i].lane < rows) {
			lane_counts[state->zombies[i].lane]++;
		}
	}

	int best_lane = 0;
	int best_count = 0x7fffffff; // i32::MAX
	const int start_lane = rows > 0 ? state->wave_runtime.next_auto_lane % rows : 0;

	for (int offset = 0; offset < rows; ++offset) {
		const int lane = (start_lane + offset) % rows;
		if ((valid_mask & (uint8_t)(1u << lane)) == 0) {
			continue;
		}
		if (lane_counts[lane] < best_count) {
			best_lane = lane;
			best_count = lane_counts[lane];
		}
	}

	state->wave_runtime.next_auto_lane = (uint8_t)((best_lane + 1) % rows);
	return best_lane;
}

static int count_group_spawns(const PvzSpawnGroup *group) { return group ? group->count : 0; }

static uint16_t count_wave_spawns(const PvzWaveDef *wave) {
	uint16_t total = 0;
	for (uint8_t i = 0; i < wave->group_count; ++i) {
		total = (uint16_t)(total + count_group_spawns(&wave->groups[i]));
	}
	return total;
}

static uint16_t level_total_weight(const PvzLevelDef *level) {
	uint16_t total = 0;
	for (uint8_t i = 0; i < level->wave_count; ++i) {
		total = (uint16_t)(total + current_wave_weight(&level->waves[i]));
	}
	return total;
}

static void remove_plant(GameState *state, int index) {
	if (index < 0 || index >= PVZ_MAX_PLANTS || !state->plants[index].active) {
		return;
	}

	BoardCoord coord = state->plants[index].coord;
	state->plant_grid[coord.row][coord.col] = -1;
	state->plants[index].active = false;
}

static bool has_zombie_ahead_in_lane(const GameState *state, int lane, int col) {
	for (int i = 0; i < PVZ_MAX_ZOMBIES; ++i) {
		if (!state->zombies[i].active || state->zombies[i].lane != lane) {
			continue;
		}
		if (state->zombies[i].x > (float)col + 0.2f) {
			return true;
		}
	}
	return false;
}

static void spawn_projectile(GameState *state, int lane, float x) {
	const int slot = get_free_projectile_slot(state);
	if (slot < 0) {
		return;
	}

	state->projectiles[slot].active = true;
	state->projectiles[slot].lane = lane;
	state->projectiles[slot].x = x;
	state->projectiles[slot].damage = state->config->pea_damage;
	state->projectiles[slot].speed = state->config->pea_speed_cells_per_second;
}

static void spawn_sun(GameState *state, BoardCoord coord) {
	const int slot = get_free_sun_slot(state);
	if (slot < 0) {
		return;
	}

	state->suns[slot].active = true;
	state->suns[slot].x = (float)coord.col + 0.5f;
	state->suns[slot].y = (float)coord.row + 0.5f;
	state->suns[slot].value = 25;
	state->suns[slot].collect_timer = state->config->sunflower_collect_delay;
}

static bool place_plant_internal(GameState *state, PlantType plant_type, BoardCoord coord, bool charge_cost) {
	const int slot = get_free_plant_slot(state);
	if (slot < 0) {
		return false;
	}

	const int cost = pvz_plant_cost(state->config, plant_type);
	if (charge_cost) {
		state->sun_count -= cost;
	}

	state->plants[slot].active = true;
	state->plants[slot].type = plant_type;
	state->plants[slot].coord = coord;
	state->plants[slot].health = get_plant_health(state, plant_type);
	// Start timer at halfway
	state->plants[slot].action_timer = (plant_type == PLANT_SUNFLOWER) ? state->config->sunflower_sun_interval * 0.5f
																	   : state->config->peashooter_fire_interval * 0.5f;
	state->plant_grid[coord.row][coord.col] = slot;
	return true;
}

static void spawn_demo_plants(GameState *state) {
	if (state->config->rows <= 0 || state->config->cols <= 0) {
		return;
	}

	BoardCoord sunflower = {0, 0};
	BoardCoord peashooter = {state->config->rows > 1 ? 1 : 0, state->config->cols > 1 ? 1 : 0};
	BoardCoord wallnut = {state->config->rows > 2 ? 2 : state->config->rows - 1,
						  state->config->cols > 2 ? 2 : state->config->cols - 1};
	place_plant_internal(state, PLANT_SUNFLOWER, sunflower, false);
	if (state->plant_grid[peashooter.row][peashooter.col] < 0) {
		place_plant_internal(state, PLANT_PEASHOOTER, peashooter, false);
	}
	if (state->plant_grid[wallnut.row][wallnut.col] < 0) {
		place_plant_internal(state, PLANT_WALLNUT, wallnut, false);
	}
}

static bool spawn_wave_zombie(GameState *state, const PvzSpawnGroup *group) {
	const int slot = get_free_zombie_slot(state);
	if (slot < 0) {
		return false;
	}

	int lane = group->lane;
	if (lane == PVZ_LANE_AUTO) {
		lane = select_auto_lane(state, group->lane_mask);
	}

	Zombie *zombie = &state->zombies[slot];
	zombie->active = true;
	zombie->type = group->type;
	zombie->lane = lane;
	zombie->x = (float)state->config->cols + 0.45f;
	zombie->health = get_zombie_health(state, group->type);
	zombie->armor = get_zombie_armor(state, group->type);
	zombie->speed = get_zombie_speed(state, group->type);
	zombie->attack_timer = state->config->zombie_attack_interval;
	return true;
}

static bool load_current_group(GameState *state) {
	// Finds and loads first non-empty group
	// Returns true if a group was found

	GameWaveRuntime *runtime = &state->wave_runtime;
	const PvzLevelDef *level = current_level_def(state);
	const PvzWaveDef *wave = &level->waves[runtime->wave_index];

	while (runtime->group_index < wave->group_count) {
		const PvzSpawnGroup *group = &wave->groups[runtime->group_index];
		if (group->count == 0) {
			runtime->group_index++;
			continue;
		}
		runtime->spawns_left_in_group = group->count;
		runtime->ticks_until_spawn = seconds_to_runtime_ticks(state, group->first_spawn_delay_sec);
		return true;
	}

	runtime->spawns_left_in_group = 0;
	runtime->ticks_until_spawn = 0;
	return false;
}

static void begin_wave(GameState *state) {
	GameWaveRuntime *runtime = &state->wave_runtime;
	const PvzLevelDef *level = current_level_def(state);
	const PvzWaveDef *wave = &level->waves[runtime->wave_index];
	runtime->wave_started = true;
	runtime->warning_active = false;
	runtime->warning_ticks_remaining = 0;
	runtime->group_index = 0;
	runtime->spawns_left_in_group = 0;
	runtime->ticks_until_spawn = 0;
	runtime->current_wave_spawns_spawned = 0;
	runtime->current_wave_spawns_total = count_wave_spawns(wave);
	load_current_group(state);
}

static void advance_to_next_wave(GameState *state) {
	GameWaveRuntime *runtime = &state->wave_runtime;
	const PvzLevelDef *level = current_level_def(state);

	// Completed waves contribute their full weight to level progress.
	if (runtime->wave_index < level->wave_count) {
		const PvzWaveDef *wave = &level->waves[runtime->wave_index];
		runtime->completed_wave_weight += current_wave_weight(wave);
	}

	runtime->wave_index++;
	runtime->wave_started = false;
	runtime->warning_active = false;
	runtime->warning_ticks_remaining = 0;
	runtime->group_index = 0;
	runtime->spawns_left_in_group = 0;
	runtime->ticks_until_spawn = 0;
	runtime->current_wave_spawns_spawned = 0;
	runtime->current_wave_spawns_total = 0;

	// Finished all waves
	if (runtime->wave_index >= level->wave_count) {
		runtime->level_exhausted = true;
		runtime->ticks_until_wave = 0;
		return;
	}

	// Setup next wave
	const PvzWaveDef *next_wave = &level->waves[runtime->wave_index];
	runtime->ticks_until_wave = seconds_to_runtime_ticks(state, next_wave->start_delay_sec);
}

static void step_wave_scheduler(GameState *state) {
	GameWaveRuntime *runtime = &state->wave_runtime;
	const PvzLevelDef *level = current_level_def(state);

	if (runtime->level_exhausted || level->wave_count == 0) {
		return;
	}

	const PvzWaveDef *wave = &level->waves[runtime->wave_index];

	if (!runtime->wave_started) {
		// Major wave warning was started
		if (runtime->warning_active) {
			if (runtime->warning_ticks_remaining > 0) {
				runtime->warning_ticks_remaining--;
			}

			// Begin major wave when zombies go below threshold
			if (runtime->warning_ticks_remaining == 0 && count_active_zombies(state) <= wave->drain_threshold) {
				begin_wave(state);
			}
			return;
		}

		if (runtime->ticks_until_wave > 0) {
			runtime->ticks_until_wave--;
			return;
		}

		// Don't start next wave if there are too many zombies
		if (count_active_zombies(state) > wave->drain_threshold) {
			return;
		}

		const uint16_t warning_ticks = seconds_to_runtime_ticks(state, PVZ_MAJOR_WAVE_WARNING_SEC);
		// If it's a major wave, then display warning
		//  The wave will then spawn when the warning ticks have been exhausted
		if (wave->flags & PVZ_WAVE_FLAG_MAJOR && warning_ticks > 0) {
			runtime->warning_active = true;
			runtime->warning_ticks_remaining = warning_ticks;
			return;
		}

		// Otherwise, begin normal wave
		begin_wave(state);
	}

	// Advance wave if all zombies in current wave have been defeated
	if (runtime->wave_started && runtime->group_index >= wave->group_count && count_active_zombies(state) == 0) {
		advance_to_next_wave(state);
		return;
	}

	// Advance wave if current wave is exhausted
	if (runtime->group_index >= wave->group_count || runtime->spawns_left_in_group == 0) {
		if (!load_current_group(state) && count_active_zombies(state) == 0) {
			advance_to_next_wave(state);
		}
		return;
	}

	if (runtime->ticks_until_spawn > 0) {
		runtime->ticks_until_spawn--;
		return;
	}

	const PvzSpawnGroup *group = &wave->groups[runtime->group_index];
	if (!spawn_wave_zombie(state, group)) {
		return;
	}

	runtime->spawns_left_in_group--;
	runtime->current_wave_spawns_spawned++;

	if (runtime->spawns_left_in_group > 0) {
		runtime->ticks_until_spawn = seconds_to_runtime_ticks(state, group->spawn_interval_sec);
		return;
	}

	runtime->group_index++;
	if (!load_current_group(state) && count_active_zombies(state) == 0) {
		advance_to_next_wave(state);
	}
}

static void damage_zombie(Zombie *zombie, int damage) {
	if (zombie->armor > 0) {
		const int absorbed = damage < zombie->armor ? damage : zombie->armor;
		zombie->armor -= absorbed;
		damage -= absorbed;
	}
	if (damage > 0) {
		zombie->health -= damage;
	}
	if (zombie->health <= 0) {
		zombie->active = false;
	}
}

static void step_suns(GameState *state, float dt) {
	for (int i = 0; i < PVZ_MAX_SUNS; ++i) {
		if (!state->suns[i].active) {
			continue;
		}
		state->suns[i].collect_timer -= dt;
		if (state->suns[i].collect_timer <= 0.0f) {
			state->sun_count += state->suns[i].value;
			state->suns[i].active = false;
		}
	}
}

static void step_plants(GameState *state, float dt) {
	for (int i = 0; i < PVZ_MAX_PLANTS; ++i) {
		if (!state->plants[i].active) {
			continue;
		}

		Plant *plant = &state->plants[i];
		if (plant->action_timer > 0) {
			plant->action_timer -= dt;
		}

		if (plant->type == PLANT_SUNFLOWER) {
			if (plant->action_timer <= 0.0f) {
				spawn_sun(state, plant->coord);
				plant->action_timer += state->config->sunflower_sun_interval;
			}
			continue;
		}

		if (plant->type == PLANT_PEASHOOTER && plant->action_timer <= 0.0f &&
			has_zombie_ahead_in_lane(state, plant->coord.row, plant->coord.col)) {
			spawn_projectile(state, plant->coord.row, (float)plant->coord.col + 0.65f);
			plant->action_timer += state->config->peashooter_fire_interval;
		}
	}
}

static void step_projectiles(GameState *state, float dt) {
	for (int i = 0; i < PVZ_MAX_PROJECTILES; ++i) {
		if (!state->projectiles[i].active) {
			continue;
		}

		Projectile *projectile = &state->projectiles[i];
		const float previous_x = projectile->x;
		projectile->x += projectile->speed * dt;

		Zombie *target = NULL;
		float best_x = 9999.0f;
		for (int j = 0; j < PVZ_MAX_ZOMBIES; ++j) {
			Zombie *zombie = &state->zombies[j];
			if (!zombie->active || zombie->lane != projectile->lane) {
				continue;
			}
			if (zombie->x + 0.35f < previous_x || zombie->x - 0.35f > projectile->x) {
				continue;
			}
			if (zombie->x < best_x) {
				best_x = zombie->x;
				target = zombie;
			}
		}

		if (target) {
			damage_zombie(target, projectile->damage);
			projectile->active = false;
			continue;
		}

		if (projectile->x > (float)state->config->cols + 0.75f) {
			projectile->active = false;
		}
	}
}

static void step_zombies(GameState *state, float dt) {
	for (int i = 0; i < PVZ_MAX_ZOMBIES; ++i) {
		if (!state->zombies[i].active) {
			continue;
		}

		Zombie *zombie = &state->zombies[i];
		const int target_col = (int)floorf(zombie->x);
		bool attacking = false;

		if (target_col >= 0 && target_col < state->config->cols && zombie->lane >= 0 &&
			zombie->lane < state->config->rows) {

			const int plant_index = state->plant_grid[zombie->lane][target_col];
			if (plant_index >= 0 && state->plants[plant_index].active && zombie->x <= (float)target_col + 0.82f) {
				attacking = true;
				zombie->attack_timer -= dt;
				if (zombie->attack_timer <= 0.0f) {
					state->plants[plant_index].health -= state->config->zombie_bite_damage;
					zombie->attack_timer += state->config->zombie_attack_interval;
					if (state->plants[plant_index].health <= 0) {
						remove_plant(state, plant_index);
					}
				}
			}
		}

		if (!attacking) {
			zombie->x -= zombie->speed * dt;
			zombie->attack_timer = state->config->zombie_attack_interval;
		}

		if (zombie->x < -0.1f) {
			state->status = GAME_STATUS_LOST;
		}
	}
}

bool game_coord_in_bounds(const GameState *state, BoardCoord coord) {
	return coord.row >= 0 && coord.row < state->config->rows && coord.col >= 0 && coord.col < state->config->cols;
}

void game_init(GameState *state, const GameConfig *config) {
	memset(state, 0, sizeof(*state));
	state->config = config;
	state->wave_runtime.level_index = 2;
	game_reset(state);
}

void game_reset(GameState *state) {
	const uint8_t level_index = clamp_level_index(state->wave_runtime.level_index);
	const PvzLevelDef *level = &builtin_levels[level_index];

	clear_state_arrays(state);
	memset(&state->wave_runtime, 0, sizeof(state->wave_runtime));
	state->wave_runtime.level_index = level_index;
	state->wave_runtime.level_total_weight = level_total_weight(level);
	state->wave_runtime.ticks_until_wave = seconds_to_runtime_ticks(state, level->opening_delay_sec);
	state->wave_runtime.next_auto_lane = 0;

	if (level->wave_count == 0) {
		state->wave_runtime.level_exhausted = true;
	}

	state->sun_count = level->starting_sun_override >= 0 ? level->starting_sun_override : state->config->starting_sun;
	state->selected_plant = PLANT_PEASHOOTER;
	state->status = GAME_STATUS_RUNNING;
	state->paused = false;
	state->last_command_result = GAME_COMMAND_RESULT_OK;

	for (int i = 0; i < PVZ_NUM_PLANT_TYPES; ++i) {
		state->seed_cooldowns[i] = 1;
	}

	if (state->config->start_with_demo_layout) {
		spawn_demo_plants(state);
	}
}

void game_set_level(GameState *state, uint8_t level_index) {
	if (!state) {
		return;
	}
	state->wave_runtime.level_index = clamp_level_index(level_index);
	if (state->config) {
		game_reset(state);
	}
}

uint8_t game_get_level_count(void) { return (uint8_t)(sizeof(builtin_levels) / sizeof(builtin_levels[0])); }

void game_get_wave_status(const GameState *state, GameWaveStatus *status) {
	if (!status) {
		return;
	}

	memset(status, 0, sizeof(*status));
	if (!state || !state->config) {
		return;
	}

	const PvzLevelDef *level = current_level_def(state);
	const GameWaveRuntime *runtime = &state->wave_runtime;

	status->wave_count = level->wave_count;
	if (runtime->level_total_weight > 0) {
		float progress_weight = (float)runtime->completed_wave_weight;
		if (runtime->wave_started && runtime->wave_index < level->wave_count &&
			runtime->current_wave_spawns_total > 0) {
			const PvzWaveDef *wave = &level->waves[runtime->wave_index];
			const float spawn_progress =
				(float)runtime->current_wave_spawns_spawned / (float)runtime->current_wave_spawns_total;
			progress_weight += (float)current_wave_weight(wave) * spawn_progress;
		}
		status->level_progress_01 = progress_weight / (float)runtime->level_total_weight;
		if (status->level_progress_01 > 1.0f) {
			status->level_progress_01 = 1.0f;
		}
	}

	uint8_t wave_index = runtime->wave_index;
	if (level->wave_count > 0 && wave_index >= level->wave_count) {
		wave_index = (uint8_t)(level->wave_count - 1);
	}
	status->current_wave_index = wave_index;
	status->warning_active = runtime->warning_active;
	status->current_wave_is_major =
		level->wave_count > 0 && (level->waves[wave_index].flags & PVZ_WAVE_FLAG_MAJOR) != 0;

	if (runtime->level_total_weight == 0) {
		return;
	}

	uint16_t cumulative_weight = 0;
	for (uint8_t i = 0; i < level->wave_count; ++i) {
		const PvzWaveDef *wave = &level->waves[i];
		if ((wave->flags & PVZ_WAVE_FLAG_MAJOR) != 0 && status->flag_marker_count < PVZ_MAX_WAVE_FLAG_MARKERS) {
			const uint32_t scaled = (uint32_t)cumulative_weight * 255u;
			status->flag_marker_progress[status->flag_marker_count++] = (uint8_t)(scaled / runtime->level_total_weight);
		}
		cumulative_weight = (uint16_t)(cumulative_weight + current_wave_weight(wave));
	}
}

GameCommandResult game_apply_command(GameState *state, GameCommand command) {
	GameCommandResult result = GAME_COMMAND_RESULT_IGNORED;

	switch (command.type) {
	case GAME_COMMAND_SELECT_PLANT:
		if (command.plant_type != PLANT_NONE) {
			state->selected_plant = command.plant_type;
			result = GAME_COMMAND_RESULT_OK;
		}
		break;
	case GAME_COMMAND_PLACE_PLANT: {
		const PlantType plant_type = command.plant_type != PLANT_NONE ? command.plant_type : state->selected_plant;
		if (plant_type == PLANT_NONE) {
			result = GAME_COMMAND_RESULT_NO_SELECTION;
			break;
		}
		if (!game_coord_in_bounds(state, command.coord)) {
			result = GAME_COMMAND_RESULT_OUT_OF_BOUNDS;
			break;
		}
		if (state->plant_grid[command.coord.row][command.coord.col] >= 0) {
			result = GAME_COMMAND_RESULT_OCCUPIED;
			break;
		}
		const int cost = pvz_plant_cost(state->config, plant_type);
		if (state->sun_count < cost) {
			result = GAME_COMMAND_RESULT_NOT_ENOUGH_SUN;
			break;
		}
		if (get_plant_seed_cooldown(state, plant_type) > 0) {
			result = GAME_COMMAND_RESULT_PLANT_ON_COOLDOWN;
			break;
		}
		if (place_plant_internal(state, plant_type, command.coord, true)) {
			activate_plant_seed_cooldown(state, plant_type);
			result = GAME_COMMAND_RESULT_OK;
		}
		break;
	}
	case GAME_COMMAND_REMOVE_PLANT:
		if (!game_coord_in_bounds(state, command.coord)) {
			result = GAME_COMMAND_RESULT_OUT_OF_BOUNDS;
			break;
		}
		if (state->plant_grid[command.coord.row][command.coord.col] < 0) {
			result = GAME_COMMAND_RESULT_NOT_FOUND;
			break;
		}
		remove_plant(state, state->plant_grid[command.coord.row][command.coord.col]);
		result = GAME_COMMAND_RESULT_OK;
		break;
	case GAME_COMMAND_TOGGLE_PAUSE:
		if (state->status == GAME_STATUS_RUNNING) {
			state->paused = !state->paused;
			result = GAME_COMMAND_RESULT_OK;
		}
		break;
	case GAME_COMMAND_RESTART:
		game_reset(state);
		result = GAME_COMMAND_RESULT_OK;
		break;
	case GAME_COMMAND_NONE:
	default:
		result = GAME_COMMAND_RESULT_IGNORED;
		break;
	}

	state->last_command_result = result;
	return result;
}

bool game_has_active_zombies(const GameState *state) { return count_active_zombies(state) > 0; }

void game_step(GameState *state, float delta) {
	if (state->paused || state->status != GAME_STATUS_RUNNING) {
		return;
	}

	step_wave_scheduler(state);
	step_suns(state, delta);
	step_plants(state, delta);
	step_projectiles(state, delta);
	step_zombies(state, delta);

	for (int i = 0; i < PVZ_NUM_PLANT_TYPES; ++i) {
		state->seed_cooldowns[i] -= delta;
		if (state->seed_cooldowns[i] < 0) {
			state->seed_cooldowns[i] = 0;
		}
	}

	if (state->status == GAME_STATUS_RUNNING && state->wave_runtime.level_exhausted &&
		!game_has_active_zombies(state)) {
		state->status = GAME_STATUS_WON;
	}
}
