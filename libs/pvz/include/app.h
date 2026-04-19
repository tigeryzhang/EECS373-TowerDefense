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
	uint8_t selected_level_index;
	uint8_t previous_level_index;
	float cycle_timer_sec;
	bool selection_dirty;
} IntroSceneState;

typedef struct {
	GameStatus outcome;
	float wipe_progress_01;
	float previous_wipe_progress_01;
	bool wipe_complete;
} ResultSceneState;

typedef struct AppContext {
	GameConfig config;
	bool quit_requested;
	SceneId active_scene_id;
	Scene *active_scene;
	Scene intro_scene;
	Scene play_scene;
	Scene result_scene;
	IntroSceneState intro_state;
	PlaySceneState play_state;
	PlayPresentationState play_presentation;
	ResultSceneState result_state;
} AppContext;

void app_init(AppContext *app, const GameConfig *config);
void app_shutdown(AppContext *app);
UpdateResult app_update(AppContext *app, const InputFrame *input, float frame_dt);
void app_prerender(AppContext *app, RenderView *view, RenderData *data);
void app_render(AppContext *app, RenderView *view, RenderData *data);
void app_request_scene(AppContext *app, SceneId next_scene);
void intro_scene_configure(Scene *scene, IntroSceneState *state);
void play_scene_configure(Scene *scene, PlaySceneState *state);
void result_scene_configure(Scene *scene, ResultSceneState *state);
