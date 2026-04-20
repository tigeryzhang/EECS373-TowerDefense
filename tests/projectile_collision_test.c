#include "game.h"
#include "pvz_config.h"

#include <stdio.h>

static int expect_true(bool condition, const char *message) {
	if (!condition) {
		fprintf(stderr, "%s\n", message);
		return 0;
	}
	return 1;
}

static void setup_collision_state(GameState *state, const GameConfig *config) {
	game_init(state, config);
	state->wave_runtime.level_exhausted = true;

	state->zombies[0] = (Zombie){
		.active = true,
		.type = ZOMBIE_REGULAR,
		.lane = 0,
		.x = 4.0f,
		.health = pvz_zombie_max_health(config, ZOMBIE_REGULAR),
		.armor = 0,
		.speed = 0.0f,
		.attack_timer = config->zombie_attack_interval,
	};

	state->projectiles[0] = (Projectile){
		.active = true,
		.lane = 0,
		.x = 3.5f,
		.damage = config->pea_damage,
		.speed = 1.0f,
	};
}

int main(void) {
	GameConfig config = pvz_make_default_config();
	GameState state;
	int zombie_health;

	pvz_clamp_config(&config);
	zombie_health = pvz_zombie_max_health(&config, ZOMBIE_REGULAR);
	setup_collision_state(&state, &config);

	game_step(&state, 0.49f);
	if (!expect_true(state.projectiles[0].active, "Projectile collided before reaching the zombie edge.")) {
		return 1;
	}
	if (!expect_true(state.zombies[0].health == zombie_health,
					 "Zombie took damage before the projectile visually reached it.")) {
		return 1;
	}

	game_step(&state, 0.02f);
	if (!expect_true(!state.projectiles[0].active, "Projectile should collide once it crosses the zombie edge.")) {
		return 1;
	}
	if (!expect_true(state.zombies[0].health == zombie_health - config.pea_damage,
					 "Zombie should take damage once the projectile crosses the zombie edge.")) {
		return 1;
	}

	puts("projectile collision test passed");
	return 0;
}
