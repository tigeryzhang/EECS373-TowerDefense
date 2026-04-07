#include "app.h"
#include "presentation.h"

#include <string.h>

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

static void play_scene_set_status(PlaySceneState *state, RenderStatus status, float duration) {
	state->status = status;
	state->status_timer = duration;
}

static void play_scene_enter(Scene *scene, AppContext *app) {
	(void)scene;
	app->play_state.accumulator = 0.0f;
}

static void play_scene_handle_command(Scene *scene, AppContext *app, InputCommand command) {
	PlaySceneState *state = (PlaySceneState *)scene->state;

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
		break;
	}
	case INPUT_COMMAND_REMOVE_TILE: {
		GameCommandResult result = game_apply_command(&state->game, (GameCommand){
																		.type = GAME_COMMAND_REMOVE_PLANT,
																		.coord = command.coord,
																	});
		play_scene_set_status(
			state, result == GAME_COMMAND_RESULT_OK ? RENDER_STATUS_REMOVED : command_result_status(result), 1.2f);
		break;
	}
	case INPUT_COMMAND_TOGGLE_PAUSE:
		game_apply_command(&state->game, (GameCommand){.type = GAME_COMMAND_TOGGLE_PAUSE});
		play_scene_set_status(state, RENDER_STATUS_NONE, 0.0f);
		break;
	case INPUT_COMMAND_RESTART:
		game_apply_command(&state->game, (GameCommand){.type = GAME_COMMAND_RESTART});
		play_scene_set_status(state, RENDER_STATUS_RESET, 1.0f);
		break;
	case INPUT_COMMAND_GOTO_PLACEHOLDER_SCENE:
		scene_request(scene, SCENE_ID_PLACEHOLDER);
		break;
	case INPUT_COMMAND_GOTO_PLAY_SCENE:
	case INPUT_COMMAND_NONE:
	default:
		break;
	}
}

static void play_scene_update(Scene *scene, AppContext *app, const InputFrame *input, float frame_dt) {
	PlaySceneState *state = (PlaySceneState *)scene->state;

	for (int i = 0; i < input->count; ++i) {
		play_scene_handle_command(scene, app, input->commands[i]);
	}

	if (state->status_timer > 0.0f) {
		state->status_timer -= frame_dt;
		if (state->status_timer <= 0.0f) {
			state->status = RENDER_STATUS_NONE;
		}
	}

	state->accumulator += frame_dt;
	while (state->accumulator >= app->config.fixed_dt) {
		game_step(&state->game, app->config.fixed_dt);
		state->accumulator -= app->config.fixed_dt;
	}
}

static void play_scene_prerender(Scene *scene, AppContext *app, RenderView *view, RenderData *data) {
	PlaySceneState *state = (PlaySceneState *)scene->state;
	(void)app;
	presentation_prerender_play_view(view, data, &state->game);
}

static void play_scene_render(Scene *scene, AppContext *app, RenderView *view, RenderData *data) {
	PlaySceneState *state = (PlaySceneState *)scene->state;
	(void)app;
	presentation_render_play_view(view, data, &state->game, state->status);
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
