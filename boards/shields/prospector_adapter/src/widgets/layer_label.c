#include "layer_label.h"

#include <ctype.h>
#include <zmk/display.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/keymap.h>

#include <fonts.h>
#include <theme.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct layer_label_state {
    zmk_keymap_layer_index_t index;
};

static void set_layer_text(lv_obj_t *label, zmk_keymap_layer_index_t index) {
    const char *name = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index));
    char text[32];
    if (name == NULL || *name == '\0') {
        snprintf(text, sizeof(text), "%d", index);
    } else {
        // Uppercase: the Gridnik font only has glyphs up to 0x60.
        size_t i = 0;
        for (; name[i] != '\0' && i < sizeof(text) - 1; i++) {
            text[i] = toupper((unsigned char)name[i]);
        }
        text[i] = '\0';
    }
    lv_label_set_text(label, text);
}

static void layer_label_update_cb(struct layer_label_state state) {
    struct zmk_widget_layer_label *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_text(widget->obj, state.index); }
}

static struct layer_label_state layer_label_get_state(const zmk_event_t *eh) {
    return (struct layer_label_state){.index = zmk_keymap_highest_layer_active()};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_label, struct layer_label_state, layer_label_update_cb,
                            layer_label_get_state)
ZMK_SUBSCRIPTION(widget_layer_label, zmk_layer_state_changed);

int zmk_widget_layer_label_init(struct zmk_widget_layer_label *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);
    lv_obj_set_style_text_font(widget->obj, &FoundryGridnikMedium_20, 0);
    lv_obj_set_style_text_color(widget->obj, lv_color_hex(THEME_LAYER_ACTIVE), 0);
    lv_label_set_text(widget->obj, "");

    sys_slist_append(&widgets, &widget->node);
    widget_layer_label_init();
    return 0;
}

lv_obj_t *zmk_widget_layer_label_obj(struct zmk_widget_layer_label *widget) { return widget->obj; }
