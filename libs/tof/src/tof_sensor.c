#include "tof_sensor.h"

#include <stddef.h>

static float tof_convert_us_to_inches(uint32_t pulse_width_us) { return pulse_width_us / 1250.0f; }

void tof_sensor_init(ToFSensor *tof) {
	if (tof == NULL)
		return;

	tof->rise_capture_us = 0;
	tof->fall_capture_us = 0;
	tof->echo_width_us = 0;
	tof->capture_state = 0;
	tof->new_measurement = 0;
	tof->distance_in = 0.0f;
	tof->hand_present = 0;
	tof->gesture_latched = 0;
	tof->close_count = 0;
}

void tof_sensor_start(ToFSensor *tof, TIM_HandleTypeDef *htim_trigger, TIM_HandleTypeDef *htim_echo) {
	(void)tof;

	HAL_TIM_PWM_Start(htim_trigger, TIM_CHANNEL_2);
	HAL_TIM_IC_Start_IT(htim_echo, TIM_CHANNEL_1);
}

void tof_sensor_input_capture_callback(ToFSensor *tof, TIM_HandleTypeDef *htim) {
	if (tof == NULL || htim == NULL)
		return;

	if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
		if (tof->capture_state == 0) {
			tof->rise_capture_us = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

			TIM2->CCER |= TIM_CCER_CC1P; // switch to falling edge
			tof->capture_state = 1;
		} else {
			tof->fall_capture_us = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
			tof->echo_width_us = tof->fall_capture_us - tof->rise_capture_us;
			tof->new_measurement = 1;

			TIM2->CCER &= ~TIM_CCER_CC1P; // switch back to rising edge
			tof->capture_state = 0;
		}
	}
}

void tof_sensor_update(ToFSensor *tof) {
	if (tof == NULL || !tof->new_measurement)
		return;

	tof->new_measurement = 0;

	if (tof->echo_width_us > 100 && tof->echo_width_us < 30000) {
		tof->distance_in = tof_convert_us_to_inches(tof->echo_width_us);
		// change this value for range approx in inches to detect hand/motion
		if (tof->distance_in <= 6.0f) {
			// change this value so that hand has to stay within range for longer/shorter to pick up sun
			if (tof->close_count < 10) {
				tof->close_count++;
			}
			// change this value to be "if(tof->close_count < _num_)" _num_ minus 1
			if (tof->close_count >= 9 && !tof->hand_present) {
				tof->hand_present = 1;
				tof->gesture_latched = 1;
			}
		} else {
			tof->close_count = 0;
			tof->hand_present = 0;
		}
	} else {
		tof->close_count = 0;
		tof->hand_present = 0;
	}
}

bool tof_sensor_consume_pickup_event(ToFSensor *tof) {
	if (tof == NULL)
		return false;

	if (tof->gesture_latched) {
		tof->gesture_latched = 0;
		return true;
	}

	return false;
}

float tof_sensor_get_distance_inches(const ToFSensor *tof) {
	if (tof == NULL)
		return 0.0f;
	return tof->distance_in;
}

bool tof_sensor_try_collect_sun(ToFSensor *tof, bool sun_present) {
	if (tof == NULL)
		return false;

	if (!sun_present) {
		// discard gesture if there is nothing to collect
		tof->gesture_latched = 0;
		return false;
	}

	return tof_sensor_consume_pickup_event(tof);
}