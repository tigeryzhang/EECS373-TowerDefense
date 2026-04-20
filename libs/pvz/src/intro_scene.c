#include "app.h"
#include "presentation.h"
#include "speaker.h"

#include <string.h>

static const float INTRO_LEVEL_CYCLE_SEC = 2.0f;

static void intro_scene_enter(Scene *scene, AppContext *app) {
	IntroSceneState *state = (IntroSceneState *)scene->state;
	(void)app;

	AUDIO_StopAllTracks();
	state->selected_level_index = 0;
	state->previous_level_index = 0;
	state->cycle_timer_sec = 0.0f;
	state->selection_dirty = false;
}

static void intro_scene_update(Scene *scene, AppContext *app, const InputFrame *input, float frame_dt) {
	IntroSceneState *state = (IntroSceneState *)scene->state;
	const uint8_t level_count = game_get_level_count();

	if (level_count == 0) {
		return;
	}

	state->cycle_timer_sec += frame_dt;
	while (state->cycle_timer_sec >= INTRO_LEVEL_CYCLE_SEC) {
		state->cycle_timer_sec -= INTRO_LEVEL_CYCLE_SEC;
		state->previous_level_index = state->selected_level_index;
		state->selected_level_index = (uint8_t)((state->selected_level_index + 1) % level_count);
		state->selection_dirty = true;
		AUDIO_PlaySFX_File("splat.wav");
	}

	for (int index = 0; index < input->count; ++index) {
		if (input->commands[index].type != INPUT_COMMAND_HAND_TRIGGER) {
			continue;
		}

		game_set_level(&app->play_state.game, state->selected_level_index);
		state->previous_level_index = state->selected_level_index;
		state->selection_dirty = false;
		scene_request(scene, SCENE_ID_PLAY);
		return;
	}
}

static void intro_scene_prerender(Scene *scene, AppContext *app, RenderView *view, RenderData *data) {
	(void)app;
	(void)data;

	IntroSceneState *state = (IntroSceneState *)scene->state;
	presentation_prerender_intro_view(view, game_get_level_count(), state->selected_level_index);
}

static void intro_scene_render(Scene *scene, AppContext *app, RenderView *view, RenderData *data) {
	(void)app;
	(void)data;

	IntroSceneState *state = (IntroSceneState *)scene->state;
	if (!state->selection_dirty) {
		return;
	}

	presentation_render_intro_view(view, game_get_level_count(), state->previous_level_index, state->selected_level_index);
	state->previous_level_index = state->selected_level_index;
	state->selection_dirty = false;
}

static void intro_scene_exit(Scene *scene, AppContext *app) {
	(void)scene;
	(void)app;
}

void intro_scene_configure(Scene *scene, IntroSceneState *state) {
	static const SceneVTable vtable = {
		.enter = intro_scene_enter,
		.update = intro_scene_update,
		.prerender = intro_scene_prerender,
		.render = intro_scene_render,
		.exit = intro_scene_exit,
	};

	scene->id = SCENE_ID_INTRO;
	scene->requested_scene = SCENE_ID_NONE;
	scene->state = state;
	scene->vtable = &vtable;
	memset(state, 0, sizeof(*state));
}
