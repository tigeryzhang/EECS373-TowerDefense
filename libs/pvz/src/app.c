#include "app.h"
#include "presentation.h"

#include <string.h>

static Scene *find_scene(AppContext *app, SceneId scene_id) {
	switch (scene_id) {
	case SCENE_ID_PLAY:
		return &app->play_scene;
	case SCENE_ID_PLACEHOLDER:
		return &app->placeholder_scene;
	case SCENE_ID_NONE:
	default:
		return NULL;
	}
}

void scene_request(Scene *scene, SceneId next_scene) {
	if (!scene) {
		return;
	}
	scene->requested_scene = next_scene;
}

void app_request_scene(AppContext *app, SceneId next_scene) {
	Scene *scene = find_scene(app, next_scene);
	if (!scene || scene == app->active_scene) {
		return;
	}

	if (app->active_scene && app->active_scene->vtable && app->active_scene->vtable->exit) {
		app->active_scene->vtable->exit(app->active_scene, app);
	}

	app->active_scene = scene;
	app->active_scene_id = next_scene;
	app->active_scene->requested_scene = SCENE_ID_NONE;
	if (app->active_scene->vtable && app->active_scene->vtable->enter) {
		app->active_scene->vtable->enter(app->active_scene, app);
	}
}

void app_init(AppContext *app, const GameConfig *config) {
	memset(app, 0, sizeof(*app));
	app->config = *config;
	pvz_clamp_config(&app->config);

	play_scene_configure(&app->play_scene, &app->play_state);
	placeholder_scene_configure(&app->placeholder_scene, &app->placeholder_state);
	game_init(&app->play_state.game, &app->config);

	app_request_scene(app, SCENE_ID_PLAY);
}

void app_shutdown(AppContext *app) { (void)app; }

UpdateResult app_update(AppContext *app, const InputFrame *input, float frame_dt) {
	if (!app->active_scene || !app->active_scene->vtable || !app->active_scene->vtable->update) {
		return UPDATE_FAILED;
	}

	app->active_scene->vtable->update(app->active_scene, app, input, frame_dt);
	if (app->active_scene->requested_scene != SCENE_ID_NONE) {
		const SceneId next_scene = app->active_scene->requested_scene;
		app->active_scene->requested_scene = SCENE_ID_NONE;
		app_request_scene(app, next_scene);
		return UPDATE_CHANGED_SCENE;
	}

	return UPDATE_NONE;
}

void app_prerender(AppContext *app, RenderView *view, RenderData *data) {
	render_view_begin(view);
	if (app->active_scene && app->active_scene->vtable && app->active_scene->vtable->prerender) {
		app->active_scene->vtable->prerender(app->active_scene, app, view, data);
	}
}

void app_render(AppContext *app, RenderView *view, RenderData *data) {
	render_view_begin(view);
	if (app->active_scene && app->active_scene->vtable && app->active_scene->vtable->render) {
		app->active_scene->vtable->render(app->active_scene, app, view, data);
	}
}
