#pragma once

#include <stdbool.h>

typedef struct {
	int x;
	int y;
	int w;
	int h;
} IntRect;

IntRect pvz_rect_make(int x, int y, int w, int h);
bool pvz_rect_is_empty(IntRect rect);
IntRect pvz_rect_intersection(IntRect a, IntRect b);
IntRect pvz_rect_union(IntRect a, IntRect b);
bool pvz_rect_contains(IntRect rect, int x, int y);
