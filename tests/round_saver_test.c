#undef NDEBUG
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "gui/round_demo.h"
#include "gui/round_saver.h"
static int64_t now_us=1000000;
int64_t esp_timer_get_time(void) {return now_us;}
void glog(const char *fmt,...) {(void)fmt;}
static unsigned flushes,waits;
static lv_disp_drv_t *pending_drv;
static lv_color_t *pending_pixels;
static lv_color_t pending_copy[1200];
static unsigned pending_count;
static void wait_dma(lv_disp_drv_t *drv) {
    assert(pending_drv==drv);
    assert(!memcmp(pending_pixels,pending_copy,pending_count*sizeof(lv_color_t)));
    pending_drv=NULL; ++waits;lv_disp_flush_ready(drv);
}
static void flush(lv_disp_drv_t *drv,const lv_area_t *a,lv_color_t *pixels) {
    assert(a->x1>=0 && a->x2<240 && a->y1>=0 && a->y2<240);
    assert((unsigned)lv_area_get_size(a)<=1200);
    if(lv_obj_get_child_cnt(lv_layer_top())) {
        uint16_t expected[1200];
        round_demo_render_effect(round_saver_effect(),expected,lv_area_get_width(a),a->x1,a->y1,a->x2,a->y2,
                          (uint32_t)(now_us/1000),LV_COLOR_16_SWAP);
        for(unsigned i=0;i<lv_area_get_size(a);i++) assert(pixels[i].full==expected[i]);
    }
    assert(!pending_drv);
    pending_drv=drv;pending_pixels=pixels;pending_count=lv_area_get_size(a);
    memcpy(pending_copy,pixels,pending_count*sizeof(lv_color_t));
    ++flushes;
}
static void tick(const void *view,bool home,bool visible,bool reduced,uint32_t idle) {
    now_us+=50000;lv_tick_inc(50);
    round_saver_tick(view,home,visible,reduced,idle);
    lv_timer_handler();
    if(pending_drv) wait_dma(pending_drv);
}
int main(int argc,char **argv) {
    (void)argv;
    lv_init();
    static lv_color_t pixels[240*5],pixels2[240*5];
    static lv_disp_draw_buf_t buf; lv_disp_draw_buf_init(&buf,pixels,argc>1?pixels2:NULL,1200);
    static lv_disp_drv_t drv; lv_disp_drv_init(&drv);
    drv.hor_res=240;drv.ver_res=240;drv.draw_buf=&buf;drv.flush_cb=flush;drv.wait_cb=wait_dma;
    lv_disp_drv_register(&drv);
    const void *home=(void *)1,*other=(void *)2;
    unsigned base=lv_obj_get_child_cnt(lv_layer_top());
    for(int i=0;i<100;i++) {
        round_saver_request(i&1?ROUND_SAVER_CREW:ROUND_SAVER_NEON);
        tick(home,true,false,false,0); /* Select without starting while locked. */
        round_saver_request(ROUND_SAVER_AUTO);
        tick(home,true,true,true,16000); /* Reduced motion suppresses auto. */
        assert(lv_obj_get_child_cnt(lv_layer_top())==base);
        tick(other,false,true,false,16000);
        assert(lv_obj_get_child_cnt(lv_layer_top())==base);
        tick(home,true,true,false,16000);
        assert(lv_obj_get_child_cnt(lv_layer_top())==base+1);
        assert(round_saver_wake(true,false)); /* Whole waking gesture consumed. */
        assert(round_saver_wake(true,false));
        assert(round_saver_wake(true,true));
        assert(!round_saver_wake(true,true));
        assert(lv_obj_get_child_cnt(lv_layer_top())==base);
        round_saver_request(ROUND_SAVER_ON); tick(other,false,true,false,0);
        assert(lv_obj_get_child_cnt(lv_layer_top())==base+1);
        tick(home,true,true,false,0); /* View change cancels. */
        assert(lv_obj_get_child_cnt(lv_layer_top())==base);
        round_saver_request(ROUND_SAVER_ON);tick(home,true,false,false,16000);
        assert(lv_obj_get_child_cnt(lv_layer_top())==base); /* Locked/dark. */
        round_saver_request(ROUND_SAVER_ON);tick(home,true,true,false,0);
        tick(home,true,false,false,16000); /* Display dims or locks. */
        assert(lv_obj_get_child_cnt(lv_layer_top())==base);
        round_saver_request(ROUND_SAVER_OFF);tick(home,true,true,false,16000);
        assert(lv_obj_get_child_cnt(lv_layer_top())==base);
    }
    assert(flushes>100 && waits==flushes);
    puts("Saver: 100 start/wake/view/lock/off cycles using real five-line LVGL rendering passed");
}
