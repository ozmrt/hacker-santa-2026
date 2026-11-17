#pragma once
#include "lvgl.h"
#include <stdbool.h>
typedef enum {INPUT_TYPE_TOUCH,INPUT_TYPE_JOYSTICK} InputType;
typedef struct {InputType type;bool is_touch_move;union {struct {int joystick_index;bool joystick_pressed;};lv_indev_data_t touch_data;} data;} InputEvent;
typedef struct View {lv_obj_t *root;void (*create)(void);void (*destroy)(void);const char *name;void (*get_hardwareinput_callback)(void **);void (*input_callback)(InputEvent *);} View;
void display_manager_switch_view(View *view);
bool display_manager_is_lockscreen_active(void);
void display_manager_set_status_bar_hidden(bool hidden);
