/*
 * The status screen.
 *
 * Two widgets over a black background: the dial, and the status text around it.
 * They overlap by design — both are full-panel layers with transparent
 * backgrounds, so each places its own content against the same coordinates
 * rather than negotiating a layout. The modifier row arrives at rung 3.4c; it
 * needs glyphs no single OFL face carries.
 *
 * The bring-up screen this replaces (corner markers, a one-pixel frame and the
 * reported resolution) did its job: orientation, offsets and extent were all
 * confirmed on hardware 24 Aug 2026. Its findings are recorded in the refork
 * spec — notably that the cover glass has rounded corners, so corner pixels sit
 * under the bezel and must never be used to diagnose geometry again.
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/display.h>

#include <focus/dial_widget.h>
#include <focus/status_widget.h>

static struct focus_widget_dial dial_widget;
static struct focus_widget_status status_widget;

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);

    focus_widget_dial_init(&dial_widget, screen);
    lv_obj_align(focus_widget_dial_obj(&dial_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    /* Second, so the text is above the disc where they meet. */
    focus_widget_status_init(&status_widget, screen);
    lv_obj_align(focus_widget_status_obj(&status_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    return screen;
}
