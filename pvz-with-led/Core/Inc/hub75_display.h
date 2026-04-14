#pragma once

#include "hub75.h"
#include "presentation.h"

// Call once at startup after HUB75_Init()
void hub75_display_init(void);

// Call every frame after app_render() to push the 
// game's board pixel buffer into the HUB75 framebuffer
void hub75_display_upload_board(const RenderView *view);