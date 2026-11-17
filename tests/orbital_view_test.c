#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "managers/views/orbital_view.h"
static int64_t now_us;
static unsigned long pixels,flushes;
static View *switched;
static bool locked;
static bool status_hidden;
void display_manager_set_status_bar_hidden(bool hidden){status_hidden=hidden;}
bool display_manager_is_lockscreen_active(void){return locked;}
View apps_menu_view;
int64_t esp_timer_get_time(void){return now_us;}
void display_manager_switch_view(View *v){switched=v;}
static void flush(lv_disp_drv_t *d,const lv_area_t *a,lv_color_t *p){(void)p;pixels+=lv_area_get_size(a);++flushes;lv_disp_flush_ready(d);}
static void touch(int x,int y,lv_indev_state_t state) {
    InputEvent e={.type=INPUT_TYPE_TOUCH,.data.touch_data={.point={x,y},.state=state}};
    orbital_view.input_callback(&e);
}
static void button(unsigned i) {
    lv_obj_t *panel=lv_obj_get_child(orbital_view.root,1);assert(panel);
    lv_obj_update_layout(orbital_view.root);
    lv_obj_t *b=lv_obj_get_child(panel,i+2);assert(b);lv_area_t a;lv_obj_get_coords(b,&a);
    int x=(a.x1+a.x2)/2,y=(a.y1+a.y2)/2;touch(x,y,LV_INDEV_STATE_PR);touch(x,y,LV_INDEV_STATE_REL);
}
static void step(unsigned ms){now_us+=(int64_t)ms*1000;lv_tick_inc(ms);lv_timer_handler();}
int main(void) {
    lv_init();static lv_color_t p[1200],q[1200];static lv_disp_draw_buf_t buf;
    lv_disp_draw_buf_init(&buf,p,q,1200);static lv_disp_drv_t d;lv_disp_drv_init(&d);
    d.hor_res=d.ver_res=240;d.draw_buf=&buf;d.flush_cb=flush;lv_disp_drv_register(&d);
    for(unsigned cycle=0;cycle<20;cycle++) {
        orbital_view.create();assert(status_hidden);step(32);button(0);step(32);
        unsigned long before=pixels;
        for(unsigned frame=0;frame<200;frame++) {
            if(frame%20==0){touch(220,80,LV_INDEV_STATE_PR);touch(220,80,LV_INDEV_STATE_REL);}
            step(16);
        }
        /* With bounded sprite invalidations, steady gameplay avoids square
         * full-frame updates. This also catches invalidation-list overflow. */
        assert(pixels-before<200ul*240*240/2);
        locked=true;step(32);locked=false;
        assert(lv_obj_get_child_cnt(orbital_view.root)==2);button(1);step(32); /* restart */
        touch(120,120,LV_INDEV_STATE_PR);touch(120,120,LV_INDEV_STATE_REL);step(32);button(2);
        assert(switched==&apps_menu_view);orbital_view.destroy();assert(!orbital_view.root);assert(!status_hidden);
    }
    printf("Orbital UI: 20 play/pause/restart/back cycles, %lu flushes, sparse redraw verified\n",flushes);
}
