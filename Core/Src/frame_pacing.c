#include "frame_pacing.h"

#include "pvz_config.h"

static float frame_pacing_fallback_dt(float fixed_dt) {
	return fixed_dt > 0.0f ? fixed_dt : PVZ_DEFAULT_FIXED_DT;
}

uint32_t frame_pacing_target_frame_ms(float fixed_dt) {
	const float safe_fixed_dt = frame_pacing_fallback_dt(fixed_dt);
	const uint32_t frame_ms = (uint32_t)(safe_fixed_dt * 1000.0f + 0.5f);
	return frame_ms > 0u ? frame_ms : 1u;
}

float frame_pacing_frame_dt_from_elapsed_ms(uint32_t elapsed_ms, float fallback_dt) {
	const float safe_fallback_dt = frame_pacing_fallback_dt(fallback_dt);
	const float frame_dt = (float)elapsed_ms / 1000.0f;

	if (frame_dt <= 0.0f || frame_dt > (safe_fallback_dt * 4.0f)) {
		return safe_fallback_dt;
	}

	return frame_dt;
}

uint32_t frame_pacing_next_deadline_ms(uint32_t previous_deadline_ms, uint32_t target_frame_ms, uint32_t now_ms) {
	const uint32_t safe_target_frame_ms = target_frame_ms > 0u ? target_frame_ms : 1u;

	if (previous_deadline_ms == 0u) {
		return now_ms + safe_target_frame_ms;
	}

	const uint32_t next_deadline_ms = previous_deadline_ms + safe_target_frame_ms;
	if ((int32_t)(now_ms - next_deadline_ms) >= 0) {
		return now_ms + safe_target_frame_ms;
	}

	return next_deadline_ms;
}
