#include "pvz_rect.h"

IntRect pvz_rect_make(int x, int y, int w, int h) { return (IntRect){.x = x, .y = y, .w = w, .h = h}; }

bool pvz_rect_is_empty(IntRect rect) { return rect.w <= 0 || rect.h <= 0; }

IntRect pvz_rect_intersection(IntRect a, IntRect b) {
	const int x0 = a.x > b.x ? a.x : b.x;
	const int y0 = a.y > b.y ? a.y : b.y;
	const int x1 = (a.x + a.w) < (b.x + b.w) ? (a.x + a.w) : (b.x + b.w);
	const int y1 = (a.y + a.h) < (b.y + b.h) ? (a.y + a.h) : (b.y + b.h);

	if (x1 <= x0 || y1 <= y0) {
		return pvz_rect_make(0, 0, 0, 0);
	}

	return pvz_rect_make(x0, y0, x1 - x0, y1 - y0);
}

IntRect pvz_rect_union(IntRect a, IntRect b) {
	if (pvz_rect_is_empty(a)) {
		return b;
	}
	if (pvz_rect_is_empty(b)) {
		return a;
	}

	const int x0 = a.x < b.x ? a.x : b.x;
	const int y0 = a.y < b.y ? a.y : b.y;
	const int x1 = (a.x + a.w) > (b.x + b.w) ? (a.x + a.w) : (b.x + b.w);
	const int y1 = (a.y + a.h) > (b.y + b.h) ? (a.y + a.h) : (b.y + b.h);

	return pvz_rect_make(x0, y0, x1 - x0, y1 - y0);
}

bool pvz_rect_contains(IntRect rect, int x, int y) {
	return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}
