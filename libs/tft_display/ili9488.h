#ifndef __ILI9488_H
#define __ILI9488_H

#include "stm32l4xx_hal.h"
#include <stdint.h>

/* === Screen Size === */
#define ILI9488_TFTWIDTH 320
#define ILI9488_TFTHEIGHT 480

/* === Pins === */
#define TFT_DC_PORT GPIOA
#define TFT_DC_PIN GPIO_PIN_1

#define TFT_CS_PORT GPIOA
#define TFT_CS_PIN GPIO_PIN_4

#define TFT_RST_PORT GPIOA
#define TFT_RST_PIN GPIO_PIN_0

/* === Control Macros === */
#define DC_COMMAND() HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_RESET)
#define DC_DATA() HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_SET)

#define CS_ACTIVE() HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_RESET)
#define CS_IDLE() HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET)

#define RST_LOW() HAL_GPIO_WritePin(TFT_RST_PORT, TFT_RST_PIN, GPIO_PIN_RESET)
#define RST_HIGH() HAL_GPIO_WritePin(TFT_RST_PORT, TFT_RST_PIN, GPIO_PIN_SET)

/* === Colors (RGB565 input) === */
#define BLACK 0x0000
#define WHITE 0xFFFF
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define YELLOW 0xFFE0

extern SPI_HandleTypeDef hspi1;

/* === API === */
void ILI9488_Init(void);
void ILI9488_SetRotation(uint8_t r);

void ILI9488_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ILI9488_FillScreen(uint16_t color);
void ILI9488_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

void ILI9488_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);

void ILI9488_WriteString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size);
void ILI9488_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *img);
/* Font */
extern const unsigned char font[];
#endif