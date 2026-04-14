#ifndef INC_HUB75_H_
#define INC_HUB75_H_

#include "main.h"
#include <stdint.h>

#define PANEL_WIDTH   64
#define PANEL_HEIGHT  32
#define NUM_ROWS      (PANEL_HEIGHT / 2)   /* 16 — 1/16 scan rate */
#define BCM_BITS      4                    /* 4-bit = 16 levels/channel */


extern uint8_t fb[PANEL_HEIGHT][PANEL_WIDTH][3];


void HUB75_Init(void);

void HUB75_ISR_Tick(void);

void HUB75_FillScreen(uint8_t r, uint8_t g, uint8_t b);
void HUB75_SetPixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);
void HUB75_Clear(void);

#endif
