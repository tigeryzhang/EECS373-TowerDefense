#pragma once

#include "game.h"
#include "presentation.h"
#include "pvz_config.h"
#include "scene.h"

typedef enum { UPDATE_NONE = 0, UPDATE_CHANGED_SCENE, UPDATE_FAILED } UpdateResult;

typedef struct {
	GameState game;
	GameState prev_game_state;
	float accumulator;
	RenderStatus status;
	float status_timer;
} PlaySceneState;

typedef struct {
	float elapsed;
} PlaceholderSceneState;

typedef struct AppContext {
	GameConfig config;
	bool quit_requested;
	SceneId active_scene_id;
	Scene *active_scene;
	Scene play_scene;
	Scene placeholder_scene;
	PlaySceneState play_state;
	PlaceholderSceneState placeholder_state;
} AppContext;

void app_init(AppContext *app, const GameConfig *config);
void app_shutdown(AppContext *app);
UpdateResult app_update(AppContext *app, const InputFrame *input, float frame_dt);
void app_prerender(AppContext *app, RenderView *view, RenderData *data);
void app_render(AppContext *app, RenderView *view, RenderData *data);
void app_request_scene(AppContext *app, SceneId next_scene);
void play_scene_configure(Scene *scene, PlaySceneState *state);
void placeholder_scene_configure(Scene *scene, PlaceholderSceneState *state);
