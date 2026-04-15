#ifndef HUB75_H
#define HUB75_H

#include <stdint.h>

void hub75_init(void);
void hub75_start(void);
void hub75_stop(void);

void hub75_set_palette_rgb565(uint8_t index, uint16_t rgb565);
void hub75_upload_indexed_64x32(const uint8_t *pixels, uint16_t stride);
void hub75_irq_handler(void);

#endif
