#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

typedef struct {
    volatile uint32_t rise_capture_us;
    volatile uint32_t fall_capture_us;
    volatile uint32_t echo_width_us;
    volatile uint8_t capture_state;
    volatile uint8_t new_measurement;

    float distance_in;
    uint8_t hand_present;
    uint8_t gesture_latched;
    uint8_t close_count;
} ToFSensor;

void tof_sensor_init(ToFSensor *tof);
void tof_sensor_start(ToFSensor *tof, TIM_HandleTypeDef *htim_trigger, TIM_HandleTypeDef *htim_echo);
void tof_sensor_input_capture_callback(ToFSensor *tof, TIM_HandleTypeDef *htim);
void tof_sensor_update(ToFSensor *tof);

//returns true once when a pickup gesture is detected
bool tof_sensor_consume_pickup_event(ToFSensor *tof);

float tof_sensor_get_distance_inches(const ToFSensor *tof);

//game logic tell sensor whether sun is available to collect
bool tof_sensor_try_collect_sun(ToFSensor *tof, bool sun_present);

