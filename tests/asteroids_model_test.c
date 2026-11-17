#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "gui/asteroids_game.h"

int main(void) {
    asteroids_game_t a, b;
    asteroids_init(&a, 42);
    asteroids_init(&b, 42);
    for (unsigned i = 0; i < 100000; i++) {
        if (!a.lives) {
            asteroids_init(&a, i + 1);
            asteroids_init(&b, i + 1);
        }
        asteroids_step(&a, i % 7 < 3, i % 51 < 25 ? -6 : 6);
        asteroids_step(&b, i % 7 < 3, i % 51 < 25 ? -6 : 6);
        assert(memcmp(&a, &b, sizeof(a)) == 0);
        assert(a.x >= 0 && a.x < 240 * 16 && a.y >= 0 && a.y < 240 * 16);
        assert(a.angle < 1024 && a.lives <= 3 && a.level >= 1);
        for (unsigned j = 0; j < ASTEROIDS_ROCKS; j++)
            if (a.rocks[j].active)
                assert(a.rocks[j].x >= 0 && a.rocks[j].x < 240 * 16 &&
                       a.rocks[j].y >= 0 && a.rocks[j].y < 240 * 16);
    }

    asteroids_init(&a, 1);
    memset(a.rocks, 0, sizeof(a.rocks));
    a.invuln = 10;
    a.fire = 10;
    a.rocks[0] = (asteroid_body_t){.x = 40 * 16, .y = 50 * 16,
                                   .size = 2, .active = 1};
    a.shots[0] = (asteroid_body_t){.x = 40 * 16, .y = 50 * 16, .active = 1};
    asteroids_step(&a, 0, 0);
    assert(a.score == 20 && !a.shots[0].active);
    assert(a.rocks[0].active && a.rocks[0].x == 40 * 16 &&
           a.rocks[0].y == 50 * 16);
    unsigned children = 0;
    for (unsigned j = 0; j < ASTEROIDS_ROCKS; j++)
        if (a.rocks[j].active) {
            assert(a.rocks[j].size == 1);
            assert(a.rocks[j].x >= 40 * 16 - 15 && a.rocks[j].x <= 40 * 16 + 15);
            assert(a.rocks[j].y >= 50 * 16 - 15 && a.rocks[j].y <= 50 * 16 + 15);
            children++;
        }
    assert(children == 2);
    printf("Asteroids: 100,000 deterministic steps, bounds and split position; model=%zu bytes\n", sizeof(a));
}
