#include "tof_sensor.h"

#include <stddef.h>
#include <string.h>

enum {
	TOF_MIN_ECHO_US = 100,
	TOF_MAX_ECHO_US = 25000,
	TOF_HAND_CLEAR_TIMEOUT_US = 250000,
	TOF_HAND_SET_SAMPLES = 2,
	TOF_HAND_CLEAR_SAMPLES = 2,
};

static const float TOF_HAND_DISTANCE_IN = 10.0f;

static uint32_t elapsed_us(uint32_t start_us, uint32_t end_us) { return end_us - start_us; }

static void set_in_range(ToFSensor *sensor) {
	sensor->out_of_range_streak = 0;
	if (sensor->in_range_streak < UINT8_MAX) {
		sensor->in_range_streak++;
	}

	if (!sensor->hand_present && sensor->in_range_streak >= TOF_HAND_SET_SAMPLES) {
		sensor->hand_present = true;
		sensor->collect_event_latched = true;
	}
}

static void set_out_of_range(ToFSensor *sensor) {
	sensor->in_range_streak = 0;
	if (sensor->out_of_range_streak < UINT8_MAX) {
		sensor->out_of_range_streak++;
	}

	if (sensor->hand_present && sensor->out_of_range_streak >= TOF_HAND_CLEAR_SAMPLES) {
		sensor->hand_present = false;
	}
}

void tof_sensor_init(ToFSensor *sensor) {
	if (sensor == NULL) {
		return;
	}

	memset(sensor, 0, sizeof(*sensor));
}

void tof_sensor_update(ToFSensor *sensor, uint32_t now_us) {
	if (sensor == NULL) {
		return;
	}

	if (sensor->pending_pulse_ready) {
		const uint32_t pulse_width_us = sensor->pending_pulse_width_us;
		sensor->pending_pulse_ready = false;
		sensor->last_measurement_us = now_us;

		if (pulse_width_us >= TOF_MIN_ECHO_US && pulse_width_us <= TOF_MAX_ECHO_US) {
			sensor->distance_in = (float)pulse_width_us / 148.0f;
			if (sensor->distance_in <= TOF_HAND_DISTANCE_IN) {
				set_in_range(sensor);
			} else {
				set_out_of_range(sensor);
			}
		} else {
			set_out_of_range(sensor);
		}
	}

	if (sensor->echo_high && elapsed_us(sensor->echo_rise_us, now_us) > TOF_MAX_ECHO_US) {
		sensor->echo_high = false;
		set_out_of_range(sensor);
	}

	if (sensor->last_measurement_us != 0u &&
		elapsed_us(sensor->last_measurement_us, now_us) > TOF_HAND_CLEAR_TIMEOUT_US) {
		set_out_of_range(sensor);
	}
}

void tof_sensor_handle_echo_edge(ToFSensor *sensor, uint32_t now_us, bool level_high) {
	if (sensor == NULL) {
		return;
	}

	if (level_high) {
		sensor->echo_rise_us = now_us;
		sensor->echo_high = true;
		return;
	}

	if (!sensor->echo_high) {
		return;
	}

	sensor->pending_pulse_width_us = elapsed_us(sensor->echo_rise_us, now_us);
	sensor->pending_pulse_ready = true;
	sensor->echo_high = false;
}

bool tof_sensor_hand_present(const ToFSensor *sensor) { return sensor != NULL && sensor->hand_present; }

bool tof_sensor_consume_collect_event(ToFSensor *sensor) {
	if (sensor == NULL || !sensor->collect_event_latched) {
		return false;
	}

	sensor->collect_event_latched = false;
	return true;
}
