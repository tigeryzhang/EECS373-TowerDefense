#pragma once

#include <stdint.h>

uint32_t frame_pacing_target_frame_ms(float fixed_dt);
float frame_pacing_frame_dt_from_elapsed_ms(uint32_t elapsed_ms, float fallback_dt);
uint32_t frame_pacing_next_deadline_ms(uint32_t previous_deadline_ms, uint32_t target_frame_ms, uint32_t now_ms);
