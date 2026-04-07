#pragma once

#include "game_types.h"
#include "pvz_config.h"

typedef enum {
	INPUT_COMMAND_NONE = 0,
	INPUT_COMMAND_SELECT_CARD,
	INPUT_COMMAND_PLACE_TILE,
	INPUT_COMMAND_REMOVE_TILE,
	INPUT_COMMAND_TOGGLE_PAUSE,
	INPUT_COMMAND_RESTART,
	INPUT_COMMAND_GOTO_PLAY_SCENE,
	INPUT_COMMAND_GOTO_PLACEHOLDER_SCENE,
} InputCommandType;

typedef struct {
	InputCommandType type;
	int index;
	BoardCoord coord;
} InputCommand;

typedef struct {
	InputCommand commands[PVZ_MAX_INPUT_COMMANDS];
	int count;
} InputFrame;

void input_frame_reset(InputFrame *frame);
bool input_frame_push(InputFrame *frame, InputCommand command);
