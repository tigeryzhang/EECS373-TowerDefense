#include "app.h"
#include "presentation.h"
#include "speaker.h"

#include <string.h>

typedef enum {
	PLAY_SCENE_SFX_NONE = 0,
	PLAY_SCENE_SFX_SPLAT,
	PLAY_SCENE_SFX_CHOMP,
	PLAY_SCENE_SFX_GULP,
	PLAY_SCENE_SFX_BUCKET,
	PLAY_SCENE_SFX_SUN,
	PLAY_SCENE_SFX_PLANT,
	PLAY_SCENE_SFX_SHOVEL,
	PLAY_SCENE_SFX_FINALWAVE,
} PlaySceneSfx;

typedef struct {
	PlaySceneSfx queued_sfx;
	bool restart_succeeded;
	bool manual_remove_succeeded;
	BoardCoord manual_remove_coord;
} PlaySceneAudioFrame;

static PlantType plant_from_card_index(int index) {
	switch (index) {
	case 0:
		return PLANT_SUNFLOWER;
	case 1:
		return PLANT_PEASHOOTER;
	case 2:
		return PLANT_WALLNUT;
	default:
		return PLANT_NONE;
	}
}

static RenderStatus command_result_status(GameCommandResult result) {
	switch (result) {
	case GAME_COMMAND_RESULT_OK:
		return RENDER_STATUS_NONE;
	case GAME_COMMAND_RESULT_OCCUPIED:
		return RENDER_STATUS_OCCUPIED;
	case GAME_COMMAND_RESULT_OUT_OF_BOUNDS:
		return RENDER_STATUS_OUT_OF_BOUNDS;
	case GAME_COMMAND_RESULT_NO_SELECTION:
		return RENDER_STATUS_NO_SELECTION;
	case GAME_COMMAND_RESULT_NOT_ENOUGH_SUN:
		return RENDER_STATUS_NO_SUN;
	case GAME_COMMAND_RESULT_NOT_FOUND:
		return RENDER_STATUS_NOT_FOUND;
	case GAME_COMMAND_RESULT_IGNORED:
	default:
		return RENDER_STATUS_NONE;
	}
}

static bool board_coord_equal(BoardCoord lhs, BoardCoord rhs) { return lhs.row == rhs.row && lhs.col == rhs.col; }

static uint8_t play_scene_sfx_priority(PlaySceneSfx sfx) {
	switch (sfx) {
	case PLAY_SCENE_SFX_FINALWAVE:
		return 7;
	case PLAY_SCENE_SFX_SUN:
	case PLAY_SCENE_SFX_PLANT:
	case PLAY_SCENE_SFX_SHOVEL:
		return 6;
	case PLAY_SCENE_SFX_BUCKET:
		return 5;
	case PLAY_SCENE_SFX_GULP:
		return 4;
	case PLAY_SCENE_SFX_CHOMP:
		return 3;
	case PLAY_SCENE_SFX_SPLAT:
		return 2;
	case PLAY_SCENE_SFX_NONE:
	default:
		return 0;
	}
}

static const char *play_scene_sfx_filename(PlaySceneSfx sfx) {
	switch (sfx) {
	case PLAY_SCENE_SFX_SPLAT:
		return "splat.wav";
	case PLAY_SCENE_SFX_CHOMP:
		return "chomp.wav";
	case PLAY_SCENE_SFX_GULP:
		return "gulp.wav";
	case PLAY_SCENE_SFX_BUCKET:
		return "bucket.wav";
	case PLAY_SCENE_SFX_SUN:
		return "sun.wav";
	case PLAY_SCENE_SFX_PLANT:
		return "plant.wav";
	case PLAY_SCENE_SFX_SHOVEL:
		return "shovel.wav";
	case PLAY_SCENE_SFX_FINALWAVE:
		return "finalwave.wav";
	case PLAY_SCENE_SFX_NONE:
	default:
		return NULL;
	}
}

static void play_scene_queue_sfx(PlaySceneAudioFrame *audio, PlaySceneSfx sfx) {
	if (!audio || sfx == PLAY_SCENE_SFX_NONE) {
		return;
	}

	if (play_scene_sfx_priority(sfx) > play_scene_sfx_priority(audio->queued_sfx)) {
		audio->queued_sfx = sfx;
	}
}

static void play_scene_flush_sfx(const PlaySceneAudioFrame *audio) {
	const char *filename = play_scene_sfx_filename(audio ? audio->queued_sfx : PLAY_SCENE_SFX_NONE);
	if (filename) {
		AUDIO_PlaySFX_File(filename);
	}
}

static void play_scene_queue_wave_sfx(const GameState *previous, const GameState *current, PlaySceneAudioFrame *audio) {
	if (!previous || !current || !audio) {
		return;
	}

	if (!previous->wave_runtime.warning_active && current->wave_runtime.warning_active) {
		play_scene_queue_sfx(audio, PLAY_SCENE_SFX_FINALWAVE);
	}
}

static void play_scene_queue_zombie_sfx(const GameState *previous, const GameState *current, PlaySceneAudioFrame *audio) {
	if (!previous || !current || !audio) {
		return;
	}

	for (int index = 0; index < PVZ_MAX_ZOMBIES; ++index) {
		const Zombie *previous_zombie = &previous->zombies[index];
		const Zombie *current_zombie = &current->zombies[index];

		if (!previous_zombie->active && current_zombie->active && current_zombie->type == ZOMBIE_BUCKETHEAD) {
			play_scene_queue_sfx(audio, PLAY_SCENE_SFX_BUCKET);
		}

		if (previous_zombie->active && current_zombie->active &&
			(current_zombie->armor < previous_zombie->armor || current_zombie->health < previous_zombie->health)) {
			play_scene_queue_sfx(audio, PLAY_SCENE_SFX_SPLAT);
		}

		if (previous_zombie->active && !current_zombie->active) {
			play_scene_queue_sfx(audio, PLAY_SCENE_SFX_SPLAT);
		}
	}
}

static void play_scene_queue_plant_sfx(const GameState *previous, const GameState *current, PlaySceneAudioFrame *audio) {
	if (!previous || !current || !audio) {
		return;
	}

	for (int index = 0; index < PVZ_MAX_PLANTS; ++index) {
		const Plant *previous_plant = &previous->plants[index];
		const Plant *current_plant = &current->plants[index];

		if (!previous_plant->active) {
			continue;
		}

		if (current_plant->active && current_plant->health < previous_plant->health) {
			play_scene_queue_sfx(audio, PLAY_SCENE_SFX_CHOMP);
			continue;
		}

		if (!current_plant->active) {
			const bool same_as_manual_remove =
				audio->manual_remove_succeeded && board_coord_equal(previous_plant->coord, audio->manual_remove_coord);
			if (!same_as_manual_remove) {
				play_scene_queue_sfx(audio, PLAY_SCENE_SFX_GULP);
			}
		}
	}
}

static void play_scene_queue_state_diff_sfx(const GameState *previous, const GameState *current,
											PlaySceneAudioFrame *audio) {
	if (!previous || !current || !audio) {
		return;
	}

	play_scene_queue_wave_sfx(previous, current, audio);
	play_scene_queue_zombie_sfx(previous, current, audio);
	play_scene_queue_plant_sfx(previous, current, audio);
}

static void play_scene_set_status(PlaySceneState *state, RenderStatus status, float duration) {
	state->status = status;
	state->status_timer = duration;
}

static void play_scene_enter(Scene *scene, AppContext *app) {
	(void)scene;

	AUDIO_StopAllTracks();
	AUDIO_PlayMusic_File("bg8bit.wav");
	app->play_state.accumulator = 0.0f;
	app->play_state.prev_game_state = app->play_state.game;
	app->play_state.status = RENDER_STATUS_NONE;
	app->play_state.status_timer = 0.0f;
}

static void play_scene_handle_command(Scene *scene, AppContext *app, InputCommand command, PlaySceneAudioFrame *audio) {
	PlaySceneState *state = (PlaySceneState *)scene->state;
	(void)app;

	switch (command.type) {
	case INPUT_COMMAND_SELECT_CARD: {
		GameCommandResult result =
			game_apply_command(&state->game, (GameCommand){
												 .type = GAME_COMMAND_SELECT_PLANT,
												 .plant_type = plant_from_card_index(command.index),
											 });
		if (result == GAME_COMMAND_RESULT_OK) {
			play_scene_set_status(state, RENDER_STATUS_NONE, 0.0f);
		}
		break;
	}
	case INPUT_COMMAND_PLACE_TILE: {
		GameCommandResult result = game_apply_command(&state->game, (GameCommand){
																		.type = GAME_COMMAND_PLACE_PLANT,
																		.coord = command.coord,
																	});
		play_scene_set_status(
			state, result == GAME_COMMAND_RESULT_OK ? RENDER_STATUS_PLACED : command_result_status(result), 1.2f);
		if (result == GAME_COMMAND_RESULT_OK) {
			play_scene_queue_sfx(audio, PLAY_SCENE_SFX_PLANT);
		}
		break;
	}
	case INPUT_COMMAND_REMOVE_TILE: {
		GameCommandResult result = game_apply_command(&state->game, (GameCommand){
																		.type = GAME_COMMAND_REMOVE_PLANT,
																		.coord = command.coord,
																	});
		play_scene_set_status(
			state, result == GAME_COMMAND_RESULT_OK ? RENDER_STATUS_REMOVED : command_result_status(result), 1.2f);
		if (result == GAME_COMMAND_RESULT_OK) {
			audio->manual_remove_succeeded = true;
			audio->manual_remove_coord = command.coord;
			play_scene_queue_sfx(audio, PLAY_SCENE_SFX_SHOVEL);
		}
		break;
	}
	case INPUT_COMMAND_COLLECT_SUN: {
		GameCommandResult result = game_apply_command(&state->game, (GameCommand){
																		.type = GAME_COMMAND_COLLECT_SUN,
																	});
		if (result == GAME_COMMAND_RESULT_OK) {
			play_scene_set_status(state, RENDER_STATUS_NONE, 0.0f);
			play_scene_queue_sfx(audio, PLAY_SCENE_SFX_SUN);
		}
		break;
	}
	case INPUT_COMMAND_TOGGLE_PAUSE:
		game_apply_command(&state->game, (GameCommand){.type = GAME_COMMAND_TOGGLE_PAUSE});
		play_scene_set_status(state, RENDER_STATUS_NONE, 0.0f);
		break;
	case INPUT_COMMAND_RESTART:
		if (game_apply_command(&state->game, (GameCommand){.type = GAME_COMMAND_RESTART}) == GAME_COMMAND_RESULT_OK) {
			audio->restart_succeeded = true;
		}
		play_scene_set_status(state, RENDER_STATUS_RESET, 1.0f);
		break;
	case INPUT_COMMAND_HAND_TRIGGER:
	case INPUT_COMMAND_NONE:
	default:
		break;
	}
}

static void play_scene_update(Scene *scene, AppContext *app, const InputFrame *input, float frame_dt) {
	PlaySceneState *state = (PlaySceneState *)scene->state;
	PlaySceneAudioFrame audio = {.queued_sfx = PLAY_SCENE_SFX_NONE, .manual_remove_coord = {0, 0}};
	state->prev_game_state = state->game;

	for (int i = 0; i < input->count; ++i) {
		play_scene_handle_command(scene, app, input->commands[i], &audio);
	}

	if (state->status_timer > 0.0f) {
		state->status_timer -= frame_dt;
		if (state->status_timer <= 0.0f) {
			state->status = RENDER_STATUS_NONE;
		}
	}

	state->accumulator += frame_dt;
	while (state->accumulator >= app->config.fixed_dt) {
		const GameState step_previous = state->game;
		game_step(&state->game, app->config.fixed_dt);
		if (!audio.restart_succeeded) {
			play_scene_queue_state_diff_sfx(&step_previous, &state->game, &audio);
		}
		state->accumulator -= app->config.fixed_dt;
	}

	if (state->game.status == GAME_STATUS_WON || state->game.status == GAME_STATUS_LOST) {
		audio.queued_sfx = PLAY_SCENE_SFX_NONE;
		app->result_state.outcome = state->game.status;
		app->result_state.wipe_progress_01 = 0.0f;
		app->result_state.previous_wipe_progress_01 = 0.0f;
		app->result_state.return_timer_sec = 0.0f;
		app->result_state.wipe_complete = false;
		scene_request(scene, SCENE_ID_RESULT);
		return;
	}

	play_scene_flush_sfx(&audio);
}

static void play_scene_prerender(Scene *scene, AppContext *app, RenderView *view, RenderData *data) {
	PlaySceneState *state = (PlaySceneState *)scene->state;
	presentation_prerender_play_view(view, data, &state->game, &app->play_presentation);
}

static void play_scene_render(Scene *scene, AppContext *app, RenderView *view, RenderData *data) {
	PlaySceneState *state = (PlaySceneState *)scene->state;
	presentation_render_play_view(view, data, &state->game, state->status, &app->play_presentation);
}

static void play_scene_exit(Scene *scene, AppContext *app) {
	(void)scene;
	(void)app;
}

void play_scene_configure(Scene *scene, PlaySceneState *state) {
	static const SceneVTable vtable = {
		.enter = play_scene_enter,
		.update = play_scene_update,
		.prerender = play_scene_prerender,
		.render = play_scene_render,
		.exit = play_scene_exit,
	};

	scene->id = SCENE_ID_PLAY;
	scene->requested_scene = SCENE_ID_NONE;
	scene->state = state;
	scene->vtable = &vtable;
	memset(state, 0, sizeof(*state));
}
