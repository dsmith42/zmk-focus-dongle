/*
 * The status text widget — the View half.
 *
 * Applies what focus_status_view_of() returned and decides nothing. Layer name,
 * profile form and battery colours were all decided by the view model and
 * host-tested there; this file turns them into LVGL calls and owns nothing but
 * placement and typeface.
 *
 * Derived from carrefinho/prospector-zmk-module (MIT) — see NOTICE.
 */

#include <focus/status_widget.h>

#include <string.h>

#include <zephyr/kernel.h>

#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

#include <focus/fonts.h>
#include <focus/status.h>
#include <focus/theme.h>

/* One number per peripheral. Two on a Corne; clamped because the view model
 * only carries what the screen has room to draw. */
#if defined(CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS)
#define STATUS_BATTERY_COUNT                                                                       \
    MIN(CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS, FOCUS_BATTERY_MAX)
#else
#define STATUS_BATTERY_COUNT 1
#endif

/*
 * Placement, in the panel's 280x240. Content is inset 10px and no corner pixel
 * is used: the cover glass is rounded, so corners sit under the bezel — proven
 * on hardware at rung 2.2 and not to be relearnt.
 *
 * The right column reads top to bottom: minutes (in the dial widget, y=22),
 * profile, batteries. Every y below is an INK position, because lv_font_conv
 * trims a subset font's line box to the glyphs it baked — so a label's box is
 * its ink and these numbers mean what they say.
 *
 *   profile   157 .. 174   (17px numerals)
 *   battery   184 .. 201   (17px numerals)
 *
 * Ten pixels between them. Profile and batteries share a face, a size and a
 * right edge with the minutes numeral above them, so the whole column is one
 * ruled line of numbers. What separates them is colour and position, not
 * decoration: the profile is grey, the batteries carry the health role.
 */
#define STATUS_PROFILE_Y 157
#define STATUS_BATTERY_Y 184
#define STATUS_BATTERY_PITCH 36
#define STATUS_INSET 10

/* The bottom row needs MORE than the standard inset, found on hardware
 * 12 Sep 2026: at 10px the corner arc ate the first character of the layer
 * name outright.
 *
 * 10px is only enough where the content is far from a corner, which is true of
 * the whole right column and false here — the row's lowest ink sits ~6px off
 * the bottom edge, and that is where the arc bites hardest. For a corner radius
 * anywhere in 20..30px the encroachment at that height is 6..10px, so doubling
 * the inset clears it with margin rather than by exactly one character.
 *
 * It applies at BOTH ends: the layer name on the left and the modifier row on
 * the right sit on the same baseline, so they meet the same two corners.
 */
#define STATUS_CORNER_INSET 20

/*
 * The modifier row, bottom right. Glyphs rather than CMD/OPT/CTL/SFT: a symbol
 * is recognised without being read, which is what the rest of this screen is
 * built around.
 *
 * Pitch is derived from the baked advance (16.8px at 28) plus air, not from the
 * old tree's 32 — that number belonged to a different face at a different size.
 *
 * ⚠️ ⌃ IS DRAWN HIGH BY ITS FACE. Measured ink above the baseline, at 28px:
 *
 *     ⌘  0..15   centre  7.5      ⌃  8..18   centre 13.0
 *     ⌥  3..12   centre  7.5      ⇧  0..16   centre  8.0
 *
 * Three of the four agree within half a pixel; U+2303 is an UP ARROWHEAD, drawn
 * at cap height like a caret rather than centred like a key symbol. Left alone
 * it floats five pixels above its neighbours. One glyph therefore carries one
 * measured offset, rather than the row being nudged by eye.
 */
#define STATUS_MODS_PITCH 26
#define STATUS_MODS_Y -8
#define STATUS_MODS_CARET_DROP 5

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

/* Peripheral battery levels arrive one event at a time and cannot be queried
 * back, so they are cached here as they come. */
static uint8_t battery_level[FOCUS_BATTERY_MAX];

static struct focus_status_view prev;
static bool have_prev;

static bool same_view(const struct focus_status_view *a, const struct focus_status_view *b) {
    if (strcmp(a->layer, b->layer) != 0 || a->layer_role != b->layer_role ||
        strcmp(a->profile, b->profile) != 0 || a->profile_role != b->profile_role ||
        a->battery_count != b->battery_count) {
        return false;
    }

    for (int i = 0; i < FOCUS_MOD_COUNT; i++) {
        if (a->mods[i] != b->mods[i]) {
            return false;
        }
    }

    for (uint8_t i = 0; i < a->battery_count; i++) {
        if (strcmp(a->battery[i].text, b->battery[i].text) != 0 ||
            a->battery[i].role != b->battery[i].role) {
            return false;
        }
    }

    return true;
}

static void render(const struct focus_status_view *v) {
    struct focus_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (!have_prev || strcmp(v->layer, prev.layer) != 0) {
            lv_label_set_text(widget->layer_label, v->layer);
        }
        if (!have_prev || v->layer_role != prev.layer_role) {
            lv_obj_set_style_text_color(
                widget->layer_label, lv_color_hex(focus_hex_of(v->layer_role, FOCUS_THEME_ANY)),
                LV_PART_MAIN);
        }

        for (int i = 0; i < FOCUS_MOD_COUNT; i++) {
            if (!have_prev || v->mods[i] != prev.mods[i]) {
                lv_obj_set_style_text_color(
                    widget->mods[i], lv_color_hex(focus_hex_of(v->mods[i], FOCUS_THEME_ANY)),
                    LV_PART_MAIN);
            }
        }

        if (!have_prev || strcmp(v->profile, prev.profile) != 0) {
            lv_label_set_text(widget->profile_label, v->profile);
        }
        if (!have_prev || v->profile_role != prev.profile_role) {
            lv_obj_set_style_text_color(
                widget->profile_label, lv_color_hex(focus_hex_of(v->profile_role, FOCUS_THEME_ANY)),
                LV_PART_MAIN);
        }

        for (uint8_t i = 0; i < v->battery_count; i++) {
            if (!have_prev || strcmp(v->battery[i].text, prev.battery[i].text) != 0) {
                lv_label_set_text(widget->battery[i], v->battery[i].text);
            }
            if (!have_prev || v->battery[i].role != prev.battery[i].role) {
                lv_obj_set_style_text_color(
                    widget->battery[i],
                    lv_color_hex(focus_hex_of(v->battery[i].role, FOCUS_THEME_ANY)), LV_PART_MAIN);
            }
        }
    }

    prev = *v;
    have_prev = true;
}

static void refresh(void) {
    struct zmk_endpoint_instance selected = zmk_endpoint_get_selected();
    zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();

    struct focus_status_state state = {
        .layer_name = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index)),
        .layer_index = index,
        .layer_uppercase = IS_ENABLED(CONFIG_FOCUS_DONGLE_LAYER_NAME_UPPERCASE),
        .mods = zmk_hid_get_explicit_mods(),
        .usb = (selected.transport == ZMK_TRANSPORT_USB),
        .profile_index = zmk_ble_active_profile_index(),
        .battery_count = STATUS_BATTERY_COUNT,
    };

    for (uint8_t i = 0; i < STATUS_BATTERY_COUNT; i++) {
        state.battery[i] = battery_level[i];
    }

    struct focus_status_view view = focus_status_view_of(&state);

    if (!have_prev || !same_view(&view, &prev)) {
        render(&view);
    }
}

/* Layer, profile, endpoint and held modifiers are all queryable at any time, so
 * one listener covers their events: each of them only says "something moved". */
struct status_state {
    uint8_t layer;
    uint8_t profile;
    uint8_t mods;
    bool usb;
};

static void status_update_cb(struct status_state state) { refresh(); }

static struct status_state status_get_state(const zmk_event_t *eh) {
    struct zmk_endpoint_instance selected = zmk_endpoint_get_selected();

    return (struct status_state){
        .layer = zmk_keymap_highest_layer_active(),
        .profile = zmk_ble_active_profile_index(),
        .mods = zmk_hid_get_explicit_mods(),
        .usb = (selected.transport == ZMK_TRANSPORT_USB),
    };
}

/* Batteries are the exception: the level arrives IN the event and there is no
 * call to read it back, so it has to be caught and kept. */
struct battery_event_state {
    uint8_t source;
    uint8_t level;
};

static void battery_update_cb(struct battery_event_state state) {
    if (state.source < FOCUS_BATTERY_MAX) {
        battery_level[state.source] = state.level;
    }
    refresh();
}

static struct battery_event_state battery_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);

    if (ev == NULL) {
        return (struct battery_event_state){.source = UINT8_MAX, .level = 0};
    }

    return (struct battery_event_state){.source = ev->source, .level = ev->state_of_charge};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_status, struct status_state, status_update_cb,
                            status_get_state)
ZMK_SUBSCRIPTION(widget_status, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(widget_status, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(widget_status, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(widget_status, zmk_keycode_state_changed);

ZMK_DISPLAY_WIDGET_LISTENER(widget_status_battery, struct battery_event_state, battery_update_cb,
                            battery_get_state)
ZMK_SUBSCRIPTION(widget_status_battery, zmk_peripheral_battery_state_changed);

static lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *font, enum focus_role role,
                            const char *text, lv_align_t align, int x, int y) {
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(focus_hex_of(role, FOCUS_THEME_ANY)),
                                LV_PART_MAIN);
    lv_obj_align(label, align, x, y);

    return label;
}

int focus_widget_status_init(struct focus_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 280, 240);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);

    widget->profile_label = make_label(widget->obj, &DINish_Medium_24, FOCUS_ROLE_GREY, "",
                                       LV_ALIGN_TOP_RIGHT, -STATUS_INSET, STATUS_PROFILE_Y);

    /* Batteries side by side, position carrying which half is which, so no L/R
     * prefixes are needed. Laid out from the right, so the pair stays against
     * the inset whether there are two peripherals or one. */
    for (int i = 0; i < STATUS_BATTERY_COUNT; i++) {
        int x = -STATUS_INSET - (STATUS_BATTERY_COUNT - 1 - i) * STATUS_BATTERY_PITCH;

        widget->battery[i] = make_label(widget->obj, &DINish_Medium_24, FOCUS_ROLE_OK, "",
                                        LV_ALIGN_TOP_RIGHT, x, STATUS_BATTERY_Y);
    }

    /* Layer bottom left, modifiers bottom right, on one baseline — a chord and
     * the layer it is on are read in a single movement. */
    widget->layer_label = make_label(widget->obj, &DINish_Medium_20, FOCUS_ROLE_ACCENT, "",
                                     LV_ALIGN_BOTTOM_LEFT, STATUS_CORNER_INSET, -6);

    static const char *const mod_glyphs[FOCUS_MOD_COUNT] = {
        [FOCUS_MOD_GUI] = "⌘",
        [FOCUS_MOD_ALT] = "⌥",
        [FOCUS_MOD_CTRL] = "⌃",
        [FOCUS_MOD_SHIFT] = "⇧",
    };

    for (int i = 0; i < FOCUS_MOD_COUNT; i++) {
        int x = -STATUS_CORNER_INSET - (FOCUS_MOD_COUNT - 1 - i) * STATUS_MODS_PITCH;
        int y = STATUS_MODS_Y + (i == FOCUS_MOD_CTRL ? STATUS_MODS_CARET_DROP : 0);

        widget->mods[i] = make_label(widget->obj, &JuliaMono_Regular_28, FOCUS_ROLE_IDLE,
                                     mod_glyphs[i], LV_ALIGN_BOTTOM_RIGHT, x, y);
    }

    sys_slist_append(&widgets, &widget->node);
    widget_status_init();
    widget_status_battery_init();

    /* Paint once now rather than waiting for the first event: a layer that is
     * never left and a profile that is never switched would otherwise leave
     * this corner blank until something happened to move. */
    refresh();

    return 0;
}

lv_obj_t *focus_widget_status_obj(struct focus_widget_status *widget) { return widget->obj; }
