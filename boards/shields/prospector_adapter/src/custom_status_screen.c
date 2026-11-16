#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

#include "widgets/layer_label.h"
#include "widgets/pet.h"
#include "widgets/battery_bar.h"
#include "widgets/output_status.h"

#include <fonts.h>
#include <theme.h>

#include <zmk/keymap.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/*
 * Layout, 240x280 portrait, top to bottom:
 *   header   layer name (left), output (right), divider below
 *   level    "LV n" + exp bar, right-aligned just under the divider
 *   pet      centered, feet on an oval shadow at PET_GROUND_Y
 *   battery  full width, one bar per half (L, R)
 */
#define MARGIN 14
#define HEADER_Y 6
#define DIVIDER_Y 36
#define LEVEL_Y (DIVIDER_Y + 4)
/* Sprites are at most 112 px tall (scripts/sprites.py MAX_SIDE), so the tallest stage's top
 * lands at y 71, clear of the level row (y 40-65). */
#define PET_GROUND_Y 183
#define BATTERY_HEIGHT 42

static struct zmk_widget_layer_label layer_label_widget;
static struct zmk_widget_pet pet_widget;
static struct zmk_widget_battery_bar battery_bar_widget;
static struct zmk_widget_output_status output_status_widget;

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen;
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, 255, LV_PART_MAIN);

    zmk_widget_layer_label_init(&layer_label_widget, screen);
    lv_obj_align(zmk_widget_layer_label_obj(&layer_label_widget), LV_ALIGN_TOP_LEFT, MARGIN, HEADER_Y);

    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_RIGHT, -MARGIN, HEADER_Y);

    lv_obj_t *divider = lv_obj_create(screen);
    lv_obj_remove_style_all(divider);
    lv_obj_set_size(divider, 240 - 2 * MARGIN, 1);
    lv_obj_set_style_bg_color(divider, lv_color_hex(THEME_DIVIDER), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    lv_obj_align(divider, LV_ALIGN_TOP_MID, 0, DIVIDER_Y);

    zmk_widget_pet_init(&pet_widget, screen);
    zmk_widget_pet_place(&pet_widget, PET_GROUND_Y);
    lv_obj_align(zmk_widget_pet_level_obj(&pet_widget), LV_ALIGN_TOP_RIGHT, -MARGIN, LEVEL_Y);

    zmk_widget_battery_bar_init(&battery_bar_widget, screen);
    lv_obj_set_size(zmk_widget_battery_bar_obj(&battery_bar_widget), lv_pct(100), BATTERY_HEIGHT);
    lv_obj_align(zmk_widget_battery_bar_obj(&battery_bar_widget), LV_ALIGN_BOTTOM_MID, 0, 0);

    return screen;
}
