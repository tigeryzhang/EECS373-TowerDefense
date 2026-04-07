#pragma once

#include "input.h"
#include "presentation.h"

struct AppContext;

typedef enum {
	SCENE_ID_NONE = 0,
	SCENE_ID_PLAY,
	SCENE_ID_PLACEHOLDER,
} SceneId;

// Forward declaration
typedef struct Scene Scene;

typedef struct {
	void (*enter)(Scene *scene, struct AppContext *app);
	void (*update)(Scene *scene, struct AppContext *app, const InputFrame *input, float frame_dt);
	void (*prerender)(Scene *scene, struct AppContext *app, RenderView *view, RenderData* data);
	void (*render)(Scene *scene, struct AppContext *app, RenderView *view, RenderData* data);
	void (*exit)(Scene *scene, struct AppContext *app);
} SceneVTable;

struct Scene {
	SceneId id;
	SceneId requested_scene;
	void *state;
	const SceneVTable *vtable;
};

void scene_request(Scene *scene, SceneId next_scene);
