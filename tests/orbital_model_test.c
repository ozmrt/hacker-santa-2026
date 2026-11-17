#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "gui/orbital_game.h"
static int abs_i(int v) {return v<0?-v:v;}
int main(void) {
    for(unsigned a=0;a<1024;a++) {
        int x,y;orbital_point(a,110*256,&x,&y);assert(x>=10 && x<=230 && y>=10 && y<=230);
        unsigned t=orbital_touch_angle(x,y);
        assert(abs_i((int)((t-a+512)&1023)-512)<=4);
    }
    assert(orbital_touch_angle(120,120)==0);
    orbital_game_t a,b;orbital_game_init(&a,3);orbital_game_init(&b,3);
    for(unsigned i=0;i<150000;i++) {
        if(!a.lives) {orbital_game_init(&a,i);orbital_game_init(&b,i);}
        a.target=b.target=(i*7)&1023;orbital_game_step(&a);orbital_game_step(&b);
        assert(!memcmp(&a,&b,sizeof(a)));assert(a.lives<=3 && a.level>=1 && a.level<=15);
        assert(a.angle<1024 && a.rng);
        for(unsigned j=0;j<ORBIT_ENEMIES;j++) if(a.enemies[j].active)assert(a.enemies[j].radius<=110*256);
    }
    orbital_game_init(&a,1);a.spawn_timer=a.fire_timer=100;
    a.enemies[0]=(orbit_body_t){500,50*256,1,1};a.shots[0]=(orbit_body_t){501,53*256,1,0};
    orbital_game_step(&a);assert(a.score==10 && !a.enemies[0].active && !a.shots[0].active);
    orbital_game_init(&a,1);a.spawn_timer=a.fire_timer=100;
    a.enemies[0]=(orbit_body_t){767,92*256-60,1,1};orbital_game_step(&a);assert(a.lives==2);
    a.angle=1020;a.target=4;orbital_game_step(&a);assert(a.angle>1019 || a.angle<5);
    printf("Orbital: 150,000 deterministic steps, bounds, angle error, collisions; model=%zu bytes\n",sizeof(a));
}
