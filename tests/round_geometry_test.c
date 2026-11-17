#include <assert.h>
#include <stdio.h>
#include "gui/round_geometry.h"

static void inside(int x, int y) {
    int dx = x - ROUND_CENTER, dy = y - ROUND_CENTER;
    assert(dx*dx + dy*dy <= ROUND_SAFE_RADIUS*ROUND_SAFE_RADIUS);
}

int main(void) {
    assert(round_band_width(120, 120) == 232);
    assert(round_band_width(0, 239) == 0);
    assert(round_band_width(150, 149) == 0);
    /* Exhaust every horizontal band: all four corners must be visible, and
     * the selected half-width must be the largest integer that fits. */
    for (int top = 5; top < 236; ++top) {
        for (int bottom = top; bottom < 236; ++bottom) {
            int half = round_band_width(top, bottom) / 2;
            inside(120-half, top);
            inside(120+half-1, top);
            inside(120-half, bottom);
            inside(120+half-1, bottom);
            int dy = 120-top > bottom-120 ? 120-top : bottom-120;
            assert((half+1)*(half+1) + dy*dy > 116*116);
        }
    }
    /* Complete key bounds on all three pages, including hidden stagger keys.
     * Use the pinned LVGL button-matrix integer layout, independently check
     * its resulting screen coordinates against the circle equation. */
    const int pages[3][4] = {{10,11,9,4}, {10,10,9,4}, {10,9,9,4}};
    for (int page = 0; page < 3; ++page) {
        for (int row = 0; row < 4; ++row) {
            int height = ROUND_KEYBOARD_HEIGHT - 3*ROUND_KEYBOARD_GAP;
            int top = ROUND_KEYBOARD_TOP + height*row/4 + row*ROUND_KEYBOARD_GAP;
            int bottom = ROUND_KEYBOARD_TOP + height*(row+1)/4 + row*ROUND_KEYBOARD_GAP - 1;
            int keys = pages[page][row];
            int spacer = round_keyboard_spacer_units(ROUND_KEYBOARD_WIDTH, keys,
                                                     ROUND_KEYBOARD_GAP, top, bottom);
            assert(spacer > 0 && spacer <= 15);
            int available = ROUND_KEYBOARD_WIDTH - ROUND_KEYBOARD_GAP*(keys+1);
            int units = 20+2*spacer;
            int left = 4 + available*spacer/units + ROUND_KEYBOARD_GAP;
            int right = 4 + available*(spacer+20)/units + keys*ROUND_KEYBOARD_GAP - 1;
            inside(left, top); inside(right, top);
            inside(left, bottom); inside(right, bottom);
            assert(available*(row == 3 ? 3 : 2)/units >= (row == 3 ? 20 : 16));
        }
    }
    puts("Round geometry: all 26796 bands and three keyboard pages fit the aperture");
}
