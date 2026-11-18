/* Keep the geometry checks active in Release builds too. */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include "gui/round_layout.h"
static void flush(lv_disp_drv_t *drv, const lv_area_t *a, lv_color_t *c) {
    (void)a; (void)c; lv_disp_flush_ready(drv);
}
int main(void) {
    lv_init();
    static lv_color_t pixels[240*5];
    static lv_disp_draw_buf_t buffer;
    lv_disp_draw_buf_init(&buffer,pixels,NULL,240*5);
    static lv_disp_drv_t drv;
    lv_disp_drv_init(&drv); drv.hor_res=240; drv.ver_res=240;
    drv.draw_buf=&buffer; drv.flush_cb=flush; lv_disp_drv_register(&drv);
    for (int cycle=0;cycle<100;cycle++) {
        lv_obj_t *list=lv_list_create(lv_scr_act());
        lv_obj_set_size(list,240,154); lv_obj_set_pos(list,0,36);
        lv_obj_set_style_pad_all(list,4,0); lv_obj_set_style_pad_row(list,2,0);
        lv_obj_set_style_border_width(list,0,0); round_list_enable(list);
        /* Exercise a short submenu between long, scrollable menus. */
        for (int pass=0;pass<3;pass++) {
            lv_obj_clean(list);
            for(int i=0;i<(pass==1?2:18);i++) {
                lv_obj_t *row=lv_list_add_btn(list,NULL,"A long option that needs horizontal scrolling");
                lv_obj_set_height(row,36);
            }
            lv_obj_update_layout(list);
            for(int y=0;y<500;y+=7) {
                lv_obj_scroll_to_y(list,y,LV_ANIM_OFF);
                lv_obj_update_layout(list);
                lv_area_t viewport; lv_obj_get_coords(list,&viewport);
                for(unsigned i=0;i<lv_obj_get_child_cnt(list);i++) {
                    lv_area_t a; lv_obj_get_coords(lv_obj_get_child(list,i),&a);
                    int top=a.y1>viewport.y1?a.y1:viewport.y1;
                    int bottom=a.y2<viewport.y2?a.y2:viewport.y2;
                    if(top>bottom) continue;
                    assert(a.x2-a.x1+1<=round_band_width(top,bottom));
                    assert(a.x1+a.x2>=238 && a.x1+a.x2<=240);
                }
            }
        }
        lv_obj_del(list);
    }
    puts("Passed 100 menu create/rebuild/scroll/destroy cycles against native LVGL");
}
