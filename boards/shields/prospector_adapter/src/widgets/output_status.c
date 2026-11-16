#include "output_status.h"

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>

#include <fonts.h>
#include <theme.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    struct zmk_endpoint_instance endpoint;
    bool connected;
    bool bonded;
};

static struct output_status_state get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .endpoint = zmk_endpoints_selected(),
        .connected = zmk_ble_active_profile_is_connected(),
        .bonded = !zmk_ble_active_profile_is_open(),
    };
}

static void set_status(struct zmk_widget_output_status *widget, struct output_status_state state) {
    if (state.endpoint.transport == ZMK_TRANSPORT_USB) {
        lv_label_set_text(widget->icon, LV_SYMBOL_USB);
        lv_label_set_text(widget->num, "");
        lv_obj_set_style_text_color(widget->icon, lv_color_hex(THEME_OUTPUT_USB), 0);
        return;
    }

    uint32_t color = THEME_OUTPUT_BLE_CONNECTED;
    const char *suffix = "";
    if (!state.bonded) {
        color = THEME_OUTPUT_BLE_OPEN;
        suffix = "?";
    } else if (!state.connected) {
        color = THEME_OUTPUT_BLE_DISCONNECTED;
    }

    // Profiles are 0-based; show them 1-based.
    lv_label_set_text(widget->icon, LV_SYMBOL_BLUETOOTH);
    lv_label_set_text_fmt(widget->num, "%d%s", state.endpoint.ble.profile_index + 1, suffix);
    lv_obj_set_style_text_color(widget->icon, lv_color_hex(color), 0);
    lv_obj_set_style_text_color(widget->num, lv_color_hex(color), 0);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status(widget, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
// Connecting or disconnecting the active profile doesn't raise endpoint_changed.
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(widget->obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(widget->obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(widget->obj, 4, 0);

    widget->icon = lv_label_create(widget->obj);
    lv_label_set_text(widget->icon, "");

    widget->num = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(widget->num, &FoundryGridnikMedium_20, 0);
    lv_label_set_text(widget->num, "");

    sys_slist_append(&widgets, &widget->node);
    widget_output_status_init();
    return 0;
}

lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget) { return widget->obj; }
