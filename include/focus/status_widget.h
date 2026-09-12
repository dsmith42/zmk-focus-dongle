/*
 * The status text widget — the View half of the pair.
 *
 * It applies what focus_status_view_of() returned and decides nothing. Every
 * string and every colour choice was made in the view model (focus/status.h),
 * which is why that half is host-testable and this half is only placement and
 * LVGL calls.
 *
 * Derived from carrefinho/prospector-zmk-module (MIT) — see NOTICE.
 * Changed: the logic this file used to carry now lives in the view model, the
 * colours arrive as roles rather than as values, and the modifier row is not
 * here yet — it needs glyphs no OFL face carries alone (rung 3.4c).
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

#include <focus/status.h>

struct focus_widget_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *layer_label;
    lv_obj_t *profile_label;
    lv_obj_t *battery[FOCUS_BATTERY_MAX];
};

int focus_widget_status_init(struct focus_widget_status *widget, lv_obj_t *parent);
lv_obj_t *focus_widget_status_obj(struct focus_widget_status *widget);
