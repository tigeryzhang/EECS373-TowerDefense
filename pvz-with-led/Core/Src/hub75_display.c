#include "hub75_display.h"
#include "presentation.h"
#include "hub75.h"

/* Palette lookup table 
Converts each RenderPalette index into 4-bit R, G, B values
(0-15) suitable for the HUB75 framebuffer 
The full 8-bit palette RGB values are defined in presentation.c
We scale them down to 4-bit by taking the top 4 bits
*/
typedef struct {uint8_t r; uint8_t g; uint8_t b;} Hub75Color;

//Pre-computed 4-bit palette — built once at init from the presentation layer's RGB values via presentation_palette_to_rgb565

static Hub75Color palette4[32];
static int palette_initialized = 0;

static void build_palette(void){
    if (palette_initialized) return;

    // We have 25 palette entries (RENDER_PALETTE_BG through RENDER_PALETTE_ART_8) 
    // Convert each one from RGB565 back to 8-bit R,G,B then scale to 4-bit
    for (int i = 0; i < 25; i++){
        uint16_t rgb565 = presentation_palette_to_rgb565((RenderPalette)i);

        // Extract 8-bit channels from RGB565
        uint8_t r8 = (uint8_t)(((rgb565 >> 11) & 0x1F) << 3);
        uint8_t g8 = (uint8_t)(((rgb565 >> 5)  & 0x3F) << 2);
        uint8_t b8 = (uint8_t)((rgb565 & 0x1F) << 3);

        // Scale to 4-bit for HUB75 framebuffer
        palette4[i].r = r8 >> 4;
        palette4[i].g = g8 >> 4;
        palette4[i].b = b8 >> 4;
    }

    palette_initialized = 1;
}

void hub75_display_init(void){
    build_palette();
    HUB75_Clear();
}


 // reads the game's board_pixels buffer and writes each pixel into the HUB75 framebuffer using the 4-bit palette
 // board_pixels is a flat array: index = y * board_width + x
 // each value is a RenderPalette enum (uint8_t, 0-24)

 void hub75_display_upload_board(const RenderView *view){
    if (!view) return;

    const int w = view->board_width < PANEL_WIDTH ? view->board_width : PANEL_WIDTH;
    const int h = view->board_height < PANEL_HEIGHT ? view->board_height : PANEL_HEIGHT;

    for (int y = 0; y < h; y++){
        for (int x = 0; x < w; x++){
            uint8_t palette_index = view->board_pixels[y * view->board_width + x];

            // guard against out-of-range palette values
            if (palette_index >= 25) palette_index = 0;

            const Hub75Color *c = &palette4[palette_index];
            HUB75_SetPixel((uint8_t)x, (uint8_t)y, c->r, c->g, c->b);
        }
    }
}