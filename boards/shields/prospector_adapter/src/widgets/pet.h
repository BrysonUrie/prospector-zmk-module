#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

/* Level L needs PET_EXP_FACTOR * (L-1)^2 key presses; evolves at PET_EVOLVE_LV1/LV2 like the games. */
#define PET_EXP_FACTOR 8
#define PET_MAX_LEVEL 100
#define PET_EVOLVE_LV1 16
#define PET_EVOLVE_LV2 36
/* The day's count resets after this long without typing. */
#define PET_IDLE_RESET_HOURS 6
/* Typing mode lasts this long after the last key press; idle plays frames PET_IDLE_SLOWDOWN times slower. */
#define PET_TYPING_HOLD_MS 1500
#define PET_IDLE_SLOWDOWN 2
/* Hop on key press: pixels up and duration of each half. */
#define PET_HOP_PX 8
#define PET_HOP_MS 90

struct zmk_widget_pet {
    lv_obj_t *obj;       /* sprite */
    lv_obj_t *shadow;    /* soft oval under the feet; image per stage from scripts/sprites.py */
    lv_obj_t *level_obj; /* "LV n" label and exp bar, placed separately */
    lv_obj_t *level_label;
    lv_obj_t *exp_bar;
    lv_timer_t *frame_timer;
    lv_timer_t *poll_timer;
    uint8_t stage;
    uint8_t frame;
    uint8_t level;
    atomic_val_t seen_presses;
    int32_t ground_y; /* screen y of the pet's feet */
};

int zmk_widget_pet_init(struct zmk_widget_pet *widget, lv_obj_t *parent);
/* Centers the pet horizontally with its feet on ground_y. Place the level row separately. */
void zmk_widget_pet_place(struct zmk_widget_pet *widget, int32_t ground_y);
lv_obj_t *zmk_widget_pet_obj(struct zmk_widget_pet *widget);
lv_obj_t *zmk_widget_pet_level_obj(struct zmk_widget_pet *widget);
