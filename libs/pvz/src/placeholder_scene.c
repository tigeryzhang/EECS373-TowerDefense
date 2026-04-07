#include "app.h"
#include "presentation.h"

static void placeholder_scene_enter(Scene *scene, AppContext *app) {
	(void)app;
	PlaceholderSceneState *state = (PlaceholderSceneState *)scene->state;
	state->elapsed = 0.0f;
}

static void placeholder_scene_update(Scene *scene, AppContext *app, const InputFrame *input, float frame_dt) {
	PlaceholderSceneState *state = (PlaceholderSceneState *)scene->state;
	state->elapsed += frame_dt;

	for (int i = 0; i < input->count; ++i) {
		switch (input->commands[i].type) {
		case INPUT_COMMAND_GOTO_PLAY_SCENE:
			scene_request(scene, SCENE_ID_PLAY);
			break;
		case INPUT_COMMAND_NONE:
		case INPUT_COMMAND_SELECT_CARD:
		case INPUT_COMMAND_PLACE_TILE:
		case INPUT_COMMAND_REMOVE_TILE:
		case INPUT_COMMAND_TOGGLE_PAUSE:
		case INPUT_COMMAND_RESTART:
		case INPUT_COMMAND_GOTO_PLACEHOLDER_SCENE:
		default:
			break;
		}
	}
}

static void placeholder_scene_build_view(Scene *scene, AppContext *app, RenderView *view, RenderData *data) {
	(void)scene;
	(void)data;
	presentation_render_placeholder_view(view, &app->config);
}

static void placeholder_scene_exit(Scene *scene, AppContext *app) {
	(void)scene;
	(void)app;
}

void placeholder_scene_configure(Scene *scene, PlaceholderSceneState *state) {
	static const SceneVTable vtable = {
		.enter = placeholder_scene_enter,
		.update = placeholder_scene_update,
		.prerender = placeholder_scene_build_view,
		.exit = placeholder_scene_exit,
	};

	scene->id = SCENE_ID_PLACEHOLDER;
	scene->requested_scene = SCENE_ID_NONE;
	scene->state = state;
	scene->vtable = &vtable;
	state->elapsed = 0.0f;
}
