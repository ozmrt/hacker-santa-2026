#include <assert.h>
#include <stdio.h>
#include "cst816_frame.h"

int main(void) {
    uint16_t x = 0, y = 0;
    assert(cst816_contact((uint8_t[]){0, 1, 0, 120, 0, 80}, &x, &y));
    assert(x == 120 && y == 80);
    assert(cst816_contact((uint8_t[]){0, 1, 0x80, 239, 0xa0, 0}, &x, &y));
    assert(x == 239 && y == 0);
    assert(!cst816_contact((uint8_t[]){0, 1, 0x40, 120, 0, 80}, &x, &y));
    assert(!cst816_contact((uint8_t[]){0, 0, 0, 120, 0, 80}, &x, &y));
    assert(!cst816_contact((uint8_t[]){0, 2, 0, 120, 0, 80}, &x, &y));
    assert(!cst816_contact((uint8_t[]){0, 1, 0xc0, 120, 0, 80}, &x, &y));
    assert(!cst816_contact((uint8_t[]){0, 1, 0, 240, 0, 80}, &x, &y));
    assert(!cst816_contact((uint8_t[]){0, 1, 1, 0, 0, 80}, &x, &y));
    assert(!cst816_contact((uint8_t[]){0, 1, 0, 120, 0, 240}, &x, &y));
    assert(!cst816_contact((uint8_t[]){0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, &x, &y));
    assert(x == 239 && y == 0); /* rejected frames cannot overwrite position */
    puts("CST816 contact tests passed");
}
