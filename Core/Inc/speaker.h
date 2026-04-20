/*
 * File: speaker.h
 */
#ifndef SPEAKER_H_
#define SPEAKER_H_

#include "stm32l4xx_hal.h"
#include <stdint.h>
#include "main.h"
#include "fatfs.h"

/* === DAC / Timer Handles (defined by CubeMX in main.c) === */
extern DAC_HandleTypeDef  hdac1;
extern TIM_HandleTypeDef  htim6;

#define AUDIO_BUFFER_SIZE   4096
#define FILE_BUFFER_SIZE  	16384


void AUDIO_Init(void);
void AUDIO_Play(void);
void AUDIO_Stop(void);
void AUDIO_PlayMusic_File(const char *filename);
void AUDIO_PlayOnce_File(const char *filename);   // one-shot, no loop
void AUDIO_PlaySFX_File(const char *filename);
void AUDIO_Task(void);
void AUDIO_PlayOnce(const char *filename);
uint8_t AUDIO_IsBGMFinished(void);               // replaces bgm_track.active

#endif /* SPEAKER_H_ */

