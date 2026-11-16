#include "pet.h"

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

#include <fonts.h>
#include <pet_sprites.h>
#include <theme.h>

#define IDLE_RESET_MS ((uint32_t)PET_IDLE_RESET_HOURS * 60 * 60 * 1000)
#define POLL_MS 30

// Key presses today and uptime of the last one; written by the event listener, read by the LVGL
// timers. Uptime restarts at boot, so a power cycle also starts a new day.
static atomic_t presses;
static atomic_t last_press_ms;

static int pet_keypress_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    uint32_t now = k_uptime_get_32();
    if (atomic_get(&presses) > 0 && now - (uint32_t)atomic_get(&last_press_ms) >= IDLE_RESET_MS) {
        atomic_set(&presses, 0);
    }
    atomic_set(&last_press_ms, now);
    atomic_inc(&presses);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(pet, pet_keypress_listener);
ZMK_SUBSCRIPTION(pet, zmk_position_state_changed);

static uint32_t exp_for_level(uint8_t level) {
    return PET_EXP_FACTOR * (uint32_t)(level - 1) * (level - 1);
}

static uint8_t level_for(atomic_val_t n) {
    uint8_t level = 1;
    while (level < PET_MAX_LEVEL && (uint32_t)n >= exp_for_level(level + 1)) {
        level++;
    }
    return level;
}

static uint8_t stage_for(uint8_t level) {
    if (level >= PET_EVOLVE_LV2) {
        return 2;
    }
    return level >= PET_EVOLVE_LV1 ? 1 : 0;
}

static bool typing(void) {
    return atomic_get(&presses) > 0 &&
           k_uptime_get_32() - (uint32_t)atomic_get(&last_press_ms) < PET_TYPING_HOLD_MS;
}

static void show_frame(struct zmk_widget_pet *widget) {
    const struct pet_sprite_stage *s = &pet_sprite_stages[widget->stage];
    uint32_t ms = s->durations_ms[widget->frame];
    lv_image_set_src(widget->obj, s->frames[widget->frame]);
    lv_timer_set_period(widget->frame_timer, typing() ? ms : ms * PET_IDLE_SLOWDOWN);
}

static void frame_tick(lv_timer_t *timer) {
    struct zmk_widget_pet *widget = lv_timer_get_user_data(timer);
    widget->frame = (widget->frame + 1) % pet_sprite_stages[widget->stage].frame_count;
    show_frame(widget);
}

// Stages differ in size, so the sprite and shadow are re-placed whenever the stage changes.
// The sprite is drawn at 2x around its center: drawn bottom = top + 1.5 * native height.
static void place_sprite(struct zmk_widget_pet *widget) {
    if (widget->ground_y == 0) {
        return; // not placed yet
    }
    const lv_image_dsc_t *f = pet_sprite_stages[widget->stage].frames[0];
    int32_t h = f->header.h;
    lv_obj_align(widget->obj, LV_ALIGN_TOP_MID, 0, widget->ground_y - h * 3 / 2);

    // Starts right at the feet; it stays put while the sprite hops.
    lv_image_set_src(widget->shadow, pet_sprite_stages[widget->stage].shadow);
    lv_obj_align(widget->shadow, LV_ALIGN_TOP_MID, 0, widget->ground_y);
}

static void set_level(struct zmk_widget_pet *widget, atomic_val_t n) {
    uint8_t level = level_for(n);
    if (level != widget->level) {
        widget->level = level;
        lv_label_set_text_fmt(widget->level_label, "LV %d", level);
    }

    if (level >= PET_MAX_LEVEL) {
        lv_bar_set_range(widget->exp_bar, 0, 1);
        lv_bar_set_value(widget->exp_bar, 1, LV_ANIM_OFF);
    } else {
        uint32_t base = exp_for_level(level);
        lv_bar_set_range(widget->exp_bar, 0, exp_for_level(level + 1) - base);
        lv_bar_set_value(widget->exp_bar, n - base, LV_ANIM_OFF);
    }

    uint8_t stage = stage_for(level);
    if (stage != widget->stage) {
        widget->stage = stage;
        widget->frame = 0;
        show_frame(widget);
        place_sprite(widget);
    }
}

static void hop_exec(void *obj, int32_t v) { lv_obj_set_style_translate_y(obj, v, 0); }

static void hop(lv_obj_t *obj) {
    if (lv_anim_get(obj, hop_exec) != NULL) {
        return; // one hop at a time, so fast typing doesn't stack them
    }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, hop_exec);
    lv_anim_set_values(&a, 0, -PET_HOP_PX);
    lv_anim_set_duration(&a, PET_HOP_MS);
    lv_anim_set_reverse_duration(&a, PET_HOP_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

// Picks up new key presses on the LVGL thread: hop, then refresh level, exp and stage.
static void poll_tick(lv_timer_t *timer) {
    struct zmk_widget_pet *widget = lv_timer_get_user_data(timer);
    atomic_val_t n = atomic_get(&presses);
    if (n == widget->seen_presses) {
        return;
    }
    widget->seen_presses = n;
    hop(widget->obj);
    set_level(widget, n);
}

int zmk_widget_pet_init(struct zmk_widget_pet *widget, lv_obj_t *parent) {
    // Created first so it draws behind the sprite.
    widget->shadow = lv_image_create(parent);

    widget->obj = lv_image_create(parent);
    lv_image_set_scale(widget->obj, 512); // 2x; frames are stored at half size to save flash
    lv_image_set_antialias(widget->obj, false);

    widget->level_obj = lv_obj_create(parent);
    lv_obj_set_size(widget->level_obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(widget->level_obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(widget->level_obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(widget->level_obj, 10, 0);

    widget->level_label = lv_label_create(widget->level_obj);
    lv_obj_set_style_text_font(widget->level_label, &FoundryGridnikMedium_20, 0);
    lv_obj_set_style_text_color(widget->level_label, lv_color_hex(THEME_LEVEL_TEXT), 0);

    widget->exp_bar = lv_bar_create(widget->level_obj);
    lv_obj_set_size(widget->exp_bar, 80, 4);
    lv_obj_set_style_radius(widget->exp_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(widget->exp_bar, 1, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(widget->exp_bar, lv_color_hex(THEME_EXP_TRACK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->exp_bar, 255, LV_PART_MAIN);
    lv_obj_set_style_bg_color(widget->exp_bar, lv_color_hex(THEME_EXP_BAR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(widget->exp_bar, 255, LV_PART_INDICATOR);

    widget->stage = 0;
    widget->frame = 0;
    widget->level = 0;
    widget->ground_y = 0;
    widget->seen_presses = atomic_get(&presses);
    widget->frame_timer = lv_timer_create(frame_tick, pet_sprite_stages[0].durations_ms[0], widget);
    widget->poll_timer = lv_timer_create(poll_tick, POLL_MS, widget);
    set_level(widget, widget->seen_presses);
    show_frame(widget);
    return 0;
}

void zmk_widget_pet_place(struct zmk_widget_pet *widget, int32_t ground_y) {
    widget->ground_y = ground_y;
    place_sprite(widget);
}

lv_obj_t *zmk_widget_pet_obj(struct zmk_widget_pet *widget) { return widget->obj; }

lv_obj_t *zmk_widget_pet_level_obj(struct zmk_widget_pet *widget) { return widget->level_obj; }
