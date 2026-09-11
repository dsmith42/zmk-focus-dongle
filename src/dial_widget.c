/*
 * The dial widget — the View half.
 *
 * Applies what focus_dial_view_of() returned and decides nothing. Every number
 * on screen was computed by the view model and host-tested there; this file
 * only turns those numbers into LVGL calls.
 *
 * Derived from carrefinho/prospector-zmk-module (MIT) — see NOTICE.
 * Changed: the arithmetic this file used to carry now lives in the view model,
 * and colours arrive as roles rather than as values.
 */

#include <focus/dial_widget.h>

#include <math.h>
#include <zephyr/kernel.h>

/* Zephyr's math.h does not expose M_PI without _GNU_SOURCE. */
#define DIAL_PI 3.14159265358979323846

#include <zmk/display.h>
#include <zmk/event_manager.h>

#include <focus/dial.h>
#include <focus/events/timer_state_changed.h>
#include <focus/timer.h>

/*
 * ONE THEME, HARDCODED — rung 3.3b.
 *
 * Themes are a later rung. The seam that makes them cheap is hex_of() below:
 * today its body returns constants, later it indexes a table built from
 * devicetree, and nothing else in this file changes. Keeping every colour
 * behind it is the whole reason multi-theme stays a one-function change.
 */
#define DIAL_WEDGE 0x1ABC9C /* teal — the public default */
#define DIAL_FACE 0x242424  /* the whole hour, behind everything */
#define DIAL_BLOCK 0x3A3A3A /* the length chosen, behind the wedge */
#define DIAL_TICK 0x9A9A9A
#define DIAL_HUB 0xD8D4CC
#define DIAL_GREY 0x8A8A8A /* the resting numeral */

/* The hand and the armed preview are DERIVED from the wedge rather than set
 * per theme: one rule for every palette, nothing to drift, and a new theme only
 * has to declare its wedge. Keep these derivations in here — let them leak into
 * the render path and the coupling the roles removed comes straight back. */
#define DIAL_HAND_LIFT 40   /* percent toward white */
#define DIAL_ARMED_LEVEL 45 /* percent of the wedge */

static uint32_t lighten(uint32_t c, int pct) {
    uint32_t r = (c >> 16) & 0xff, g = (c >> 8) & 0xff, b = c & 0xff;

    r += ((255 - r) * pct) / 100;
    g += ((255 - g) * pct) / 100;
    b += ((255 - b) * pct) / 100;

    return (r << 16) | (g << 8) | b;
}

static uint32_t scale(uint32_t c, int pct) {
    uint32_t r = (((c >> 16) & 0xff) * pct) / 100;
    uint32_t g = (((c >> 8) & 0xff) * pct) / 100;
    uint32_t b = ((c & 0xff) * pct) / 100;

    return (r << 16) | (g << 8) | b;
}

/* The single seam between roles and values. */
static uint32_t hex_of(enum focus_role role) {
    switch (role) {
    case FOCUS_ROLE_THEME:
        return DIAL_WEDGE;
    case FOCUS_ROLE_DIM:
        return scale(DIAL_WEDGE, DIAL_ARMED_LEVEL);
    case FOCUS_ROLE_GREY:
    default:
        return DIAL_GREY;
    }
}

/* The hand follows the wedge's role so that an armed preview dims as one piece
 * rather than a bright hand over a dim wedge. */
static uint32_t hand_hex_of(enum focus_role wedge_role) {
    if (wedge_role == FOCUS_ROLE_THEME) {
        return lighten(DIAL_WEDGE, DIAL_HAND_LIFT);
    }
    return hex_of(wedge_role);
}

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static struct k_work_delayable dial_tick_work;

/* One previous view instead of seven loose statics. Comparing whole views is
 * also what keeps the "did anything change" question honest: a field added to
 * the view model has to be handled here or the compiler is no help. */
static struct focus_dial_view prev;
static bool have_prev;

static bool same_view(const struct focus_dial_view *a, const struct focus_dial_view *b) {
    return a->numeral == b->numeral && a->numeral_role == b->numeral_role &&
           a->hand_deg == b->hand_deg && a->track_deg == b->track_deg &&
           a->wedge_deg == b->wedge_deg && a->ring_visible == b->ring_visible &&
           a->ring_track_deg == b->ring_track_deg && a->ring_wedge_deg == b->ring_wedge_deg &&
           a->wedge_role == b->wedge_role;
}

static void render(const struct focus_dial_view *v) {
    struct focus_widget_dial *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (!have_prev || v->wedge_role != prev.wedge_role) {
            lv_color_t wedge = lv_color_hex(hex_of(v->wedge_role));
            lv_obj_set_style_arc_color(widget->arc, wedge, LV_PART_INDICATOR);
            lv_obj_set_style_arc_color(widget->ring, wedge, LV_PART_INDICATOR);
            lv_obj_set_style_line_color(widget->hand,
                                        lv_color_hex(hand_hex_of(v->wedge_role)), LV_PART_MAIN);
        }

        if (!have_prev || v->numeral_role != prev.numeral_role) {
            lv_obj_set_style_text_color(widget->minutes_label,
                                        lv_color_hex(hex_of(v->numeral_role)), LV_PART_MAIN);
        }

        /* Grey track spans the BLOCK, not the face, so a 45 leaves a black
         * quarter and the finished screen still shows how long the block was. */
        if (!have_prev || v->track_deg != prev.track_deg) {
            lv_arc_set_bg_angles(widget->arc, 0, v->track_deg);
        }
        if (!have_prev || v->wedge_deg != prev.wedge_deg) {
            lv_arc_set_angles(widget->arc, 0, v->wedge_deg);
        }

        if (!have_prev || v->ring_visible != prev.ring_visible) {
            if (v->ring_visible) {
                lv_obj_clear_flag(widget->ring, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(widget->face_ring, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(widget->ring, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(widget->face_ring, LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (!have_prev || v->ring_track_deg != prev.ring_track_deg) {
            lv_arc_set_bg_angles(widget->ring, 0, v->ring_track_deg);
        }
        if (!have_prev || v->ring_wedge_deg != prev.ring_wedge_deg) {
            lv_arc_set_angles(widget->ring, 0, v->ring_wedge_deg);
        }

        if (!have_prev || v->hand_deg != prev.hand_deg) {
            double rad = (v->hand_deg - 90.0) * DIAL_PI / 180.0;
            widget->hand_points[1].x = DIAL_CX + DIAL_HAND_R * cos(rad);
            widget->hand_points[1].y = DIAL_CY + DIAL_HAND_R * sin(rad);
            lv_line_set_points(widget->hand, widget->hand_points, 2);
        }

        if (!have_prev || v->numeral != prev.numeral) {
            char text[8];
            snprintf(text, sizeof(text), "%d", v->numeral);
            lv_label_set_text(widget->minutes_label, text);
        }
    }

    prev = *v;
    have_prev = true;
}

static void refresh(void) {
    struct focus_timer_state state;
    focus_timer_get(&state);

    struct focus_dial_view view = focus_dial_view_of(&state);

    if (!have_prev || !same_view(&view, &prev)) {
        render(&view);
    }

    /* Keep ticking for as long as a block is LIVE — which includes overrunning.
     * The old fork stopped at zero; this design counts up past it and stays
     * locked until deliberately stopped, so stopping the tick here would freeze
     * the overrun at the moment it became interesting. */
    if (state.running) {
        k_work_schedule(&dial_tick_work, K_SECONDS(1));
    }
}

static void dial_tick_work_handler(struct k_work *work) { refresh(); }

struct dial_state {
    bool running;
};

static void dial_update_cb(struct dial_state state) {
    k_work_cancel_delayable(&dial_tick_work);
    refresh();
}

static struct dial_state dial_get_state(const zmk_event_t *eh) {
    struct focus_timer_state state;
    focus_timer_get(&state);

    return (struct dial_state){.running = state.running};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_dial, struct dial_state, dial_update_cb, dial_get_state)
ZMK_SUBSCRIPTION(widget_dial, focus_timer_state_changed);

int focus_widget_dial_init(struct focus_widget_dial *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 280, 240);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);

    /* Ticks first so the disc paints over their inner ends. */
    for (int i = 0; i < DIAL_TICK_COUNT; i++) {
        double rad = (360.0 * i / DIAL_TICK_COUNT - 90.0) * DIAL_PI / 180.0;
        int len = DIAL_TICK_OUT - DIAL_TICK_IN;
        int mx = DIAL_CX + (DIAL_TICK_IN + len / 2) * cos(rad);
        int my = DIAL_CY + (DIAL_TICK_IN + len / 2) * sin(rad);

        widget->ticks[i] = lv_obj_create(widget->obj);
        lv_obj_set_size(widget->ticks[i], 3, 3);
        lv_obj_set_pos(widget->ticks[i], mx - 1, my - 1);
        lv_obj_set_style_bg_color(widget->ticks[i], lv_color_hex(DIAL_TICK), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(widget->ticks[i], LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(widget->ticks[i], 0, LV_PART_MAIN);
        lv_obj_set_style_radius(widget->ticks[i], 2, LV_PART_MAIN);
        lv_obj_set_style_pad_all(widget->ticks[i], 0, LV_PART_MAIN);
    }

    /* Face: the whole hour, always full, behind the block track. Without it the
     * unused part of a short block reads as a missing chunk rather than as part
     * of a dial. */
    widget->face = lv_obj_create(widget->obj);
    lv_obj_set_size(widget->face, DIAL_R * 2, DIAL_R * 2);
    lv_obj_set_pos(widget->face, DIAL_CX - DIAL_R, DIAL_CY - DIAL_R);
    lv_obj_set_style_radius(widget->face, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(widget->face, lv_color_hex(DIAL_FACE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->face, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->face, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->face, 0, LV_PART_MAIN);

    widget->face_ring = lv_arc_create(widget->obj);
    lv_obj_remove_style(widget->face_ring, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(widget->face_ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(widget->face_ring, DIAL_RING_R * 2, DIAL_RING_R * 2);
    lv_obj_set_pos(widget->face_ring, DIAL_CX - DIAL_RING_R, DIAL_CY - DIAL_RING_R);
    lv_arc_set_rotation(widget->face_ring, 270);
    lv_arc_set_bg_angles(widget->face_ring, 0, 360);
    lv_arc_set_angles(widget->face_ring, 0, 0);
    lv_obj_set_style_arc_width(widget->face_ring, DIAL_RING_W, LV_PART_MAIN);
    lv_obj_set_style_arc_color(widget->face_ring, lv_color_hex(DIAL_FACE), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(widget->face_ring, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(widget->face_ring, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->face_ring, 0, LV_PART_MAIN);
    lv_obj_add_flag(widget->face_ring, LV_OBJ_FLAG_HIDDEN);

    /* An arc whose width equals its radius renders as a filled pie sector,
     * which is how you get a wedge out of LVGL without a canvas. */
    widget->arc = lv_arc_create(widget->obj);
    lv_obj_remove_style(widget->arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(widget->arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(widget->arc, DIAL_R * 2, DIAL_R * 2);
    lv_obj_set_pos(widget->arc, DIAL_CX - DIAL_R, DIAL_CY - DIAL_R);
    lv_arc_set_rotation(widget->arc, 270);
    lv_arc_set_bg_angles(widget->arc, 0, 0);
    lv_arc_set_angles(widget->arc, 0, 0);
    lv_obj_set_style_arc_width(widget->arc, DIAL_R, LV_PART_MAIN);
    lv_obj_set_style_arc_width(widget->arc, DIAL_R, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(widget->arc, lv_color_hex(DIAL_BLOCK), LV_PART_MAIN);
    lv_obj_set_style_arc_color(widget->arc, lv_color_hex(DIAL_WEDGE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(widget->arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->arc, 0, LV_PART_MAIN);

    widget->ring = lv_arc_create(widget->obj);
    lv_obj_remove_style(widget->ring, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(widget->ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(widget->ring, DIAL_RING_R * 2, DIAL_RING_R * 2);
    lv_obj_set_pos(widget->ring, DIAL_CX - DIAL_RING_R, DIAL_CY - DIAL_RING_R);
    lv_arc_set_rotation(widget->ring, 270);
    lv_arc_set_bg_angles(widget->ring, 0, 0);
    lv_arc_set_angles(widget->ring, 0, 0);
    lv_obj_set_style_arc_width(widget->ring, DIAL_RING_W, LV_PART_MAIN);
    lv_obj_set_style_arc_width(widget->ring, DIAL_RING_W, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(widget->ring, lv_color_hex(DIAL_BLOCK), LV_PART_MAIN);
    lv_obj_set_style_arc_color(widget->ring, lv_color_hex(DIAL_WEDGE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(widget->ring, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->ring, 0, LV_PART_MAIN);
    lv_obj_add_flag(widget->ring, LV_OBJ_FLAG_HIDDEN);

    widget->hand_points[0].x = DIAL_CX;
    widget->hand_points[0].y = DIAL_CY;
    widget->hand_points[1].x = DIAL_CX;
    widget->hand_points[1].y = DIAL_CY - DIAL_HAND_R;

    widget->hand = lv_line_create(widget->obj);
    lv_line_set_points(widget->hand, widget->hand_points, 2);
    lv_obj_set_style_line_width(widget->hand, 4, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(widget->hand, true, LV_PART_MAIN);
    lv_obj_set_style_line_color(widget->hand, lv_color_hex(hand_hex_of(FOCUS_ROLE_THEME)),
                                LV_PART_MAIN);

    widget->hub = lv_obj_create(widget->obj);
    lv_obj_set_size(widget->hub, DIAL_HUB_R * 2, DIAL_HUB_R * 2);
    lv_obj_set_pos(widget->hub, DIAL_CX - DIAL_HUB_R, DIAL_CY - DIAL_HUB_R);
    lv_obj_set_style_radius(widget->hub, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(widget->hub, lv_color_hex(DIAL_HUB), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->hub, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->hub, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->hub, 0, LV_PART_MAIN);

    /* Minutes top-right. Muted: a check on the dial, not the display.
     *
     * Montserrat is a PLACEHOLDER — LVGL ships it, so the geometry rung needs
     * no font work at all. Rung 3.4 replaces it with a DINish cut, which is the
     * OFL face already earmarked for every Latin glyph on this screen. */
    widget->minutes_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->minutes_label, "0");
    lv_obj_set_style_text_font(widget->minutes_label, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->minutes_label, lv_color_hex(DIAL_GREY), LV_PART_MAIN);
    lv_obj_align(widget->minutes_label, LV_ALIGN_TOP_RIGHT, -10, 22);

    sys_slist_append(&widgets, &widget->node);
    widget_dial_init();

    k_work_init_delayable(&dial_tick_work, dial_tick_work_handler);

    /* Paint once now rather than waiting for the first event, so a dongle that
     * boots with no timer armed still shows a dial rather than a blank face. */
    refresh();

    return 0;
}

lv_obj_t *focus_widget_dial_obj(struct focus_widget_dial *widget) { return widget->obj; }
