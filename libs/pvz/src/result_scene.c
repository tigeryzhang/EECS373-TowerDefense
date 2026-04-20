#include "app.h"
#include "presentation.h"
#include "speaker.h"

#include <string.h>

static const float RESULT_WIPE_DURATION_SEC = 0.75f;
static const float RESULT_RETURN_DELAY_SEC = 15.0f;

static void result_scene_enter(Scene *scene, AppContext *app) {
	ResultSceneState *state = (ResultSceneState *)scene->state;

	state->wipe_progress_01 = 0.0f;
	state->previous_wipe_progress_01 = 0.0f;
	state->previous_return_timer_sec = 0.0f;
	state->return_timer_sec = 0.0f;
	state->wipe_complete = false;

	if (app->result_state.outcome == GAME_STATUS_WON) {
		AUDIO_PlayOnce_File("winmusic.wav");
	} else {
		AUDIO_PlayOnce_File("lose.wav");
	}

	app->intro_state.selected_level_index = 0;
	app->intro_state.previous_level_index = 0;
	app->intro_state.cycle_timer_sec = 0.0f;
	app->intro_state.selection_dirty = false;
}

static void result_scene_update(Scene *scene, AppContext *app, const InputFrame *input, float frame_dt) {
	(void)input;
	(void)app;

	ResultSceneState *state = (ResultSceneState *)scene->state;
	state->previous_wipe_progress_01 = state->wipe_progress_01;
	state->previous_return_timer_sec = state->return_timer_sec;
	state->return_timer_sec += frame_dt;

	if (!state->wipe_complete) {
		state->wipe_progress_01 += frame_dt / RESULT_WIPE_DURATION_SEC;
		if (state->wipe_progress_01 >= 1.0f) {
			state->wipe_progress_01 = 1.0f;
			state->wipe_complete = true;
		}
	}

	if (state->return_timer_sec >= RESULT_RETURN_DELAY_SEC) {
		scene_request(scene, SCENE_ID_INTRO);
	}
}

static void result_scene_prerender(Scene *scene, AppContext *app, RenderView *view, RenderData *data) {
	(void)scene;
	(void)app;
	(void)data;
	presentation_prerender_result_view(view);
}

static void result_scene_render(Scene *scene, AppContext *app, RenderView *view, RenderData *data) {
	(void)app;
	(void)data;

	ResultSceneState *state = (ResultSceneState *)scene->state;
	presentation_render_result_view(view, state->outcome, state->previous_wipe_progress_01, state->wipe_progress_01,
									state->previous_return_timer_sec, state->return_timer_sec);
}

static void result_scene_exit(Scene *scene, AppContext *app) {
	(void)scene;
	(void)app;
}

void result_scene_configure(Scene *scene, ResultSceneState *state) {
	static const SceneVTable vtable = {
		.enter = result_scene_enter,
		.update = result_scene_update,
		.prerender = result_scene_prerender,
		.render = result_scene_render,
		.exit = result_scene_exit,
	};

	scene->id = SCENE_ID_RESULT;
	scene->requested_scene = SCENE_ID_NONE;
	scene->state = state;
	scene->vtable = &vtable;
	memset(state, 0, sizeof(*state));
	state->outcome = GAME_STATUS_LOST;
}
