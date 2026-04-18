#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	volatile bool echo_high;
	volatile bool pending_pulse_ready;
	volatile uint32_t echo_rise_us;
	volatile uint32_t pending_pulse_width_us;
	float distance_in;
	uint32_t last_measurement_us;
	uint8_t in_range_streak;
	uint8_t out_of_range_streak;
	bool hand_present;
	bool collect_event_latched;
} ToFSensor;

void tof_sensor_init(ToFSensor *sensor);
void tof_sensor_update(ToFSensor *sensor, uint32_t now_us);
void tof_sensor_handle_echo_edge(ToFSensor *sensor, uint32_t now_us, bool level_high);
bool tof_sensor_hand_present(const ToFSensor *sensor);
bool tof_sensor_consume_collect_event(ToFSensor *sensor);
