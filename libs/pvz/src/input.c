#include "input.h"

void input_frame_reset(InputFrame *frame) {
	if (!frame) {
		return;
	}

	frame->count = 0;
}

bool input_frame_push(InputFrame *frame, InputCommand command) {
	if (!frame || frame->count >= PVZ_MAX_INPUT_COMMANDS) {
		return false;
	}

	frame->commands[frame->count++] = command;
	return true;
}
