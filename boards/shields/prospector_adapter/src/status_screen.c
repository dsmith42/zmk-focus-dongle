/*
 * Bring-up screen.
 *
 * Deliberately diagnostic rather than decorative. It answers the three questions
 * this rung exists to ask, and it answers them by being wrong in an obvious way
 * if anything is off:
 *
 *   Orientation  the panel is a 240x280 portrait module driven in landscape, so
 *                the driver is asked for ROTATED_270 at init. The reported
 *                resolution is printed: 280x240 means the rotation took, 240x280
 *                means it did not. Upstream Zephyr's driver returns -ENOTSUP
 *                here, which is why a modified copy is vendored.
 *
 *   Offsets      the visible area sits at an offset inside the controller's RAM
 *                (y-offset 20). Corner markers sit hard against all four edges,
 *                so a wrong offset shows as a missing or clipped marker rather
 *                than as something subtly off-centre.
 *
 *   Extent       a one-pixel frame around the whole screen. If any edge of it is
 *                missing, the drawable area is not where we think it is.
 *
 * Replaced by the real dial once this is confirmed on hardware.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <zmk/display.h>

#define MARKER 12

static void corner(lv_obj_t *parent, lv_align_t align) {
    lv_obj_t *m = lv_obj_create(parent);
    lv_obj_set_size(m, MARKER, MARKER);
    lv_obj_align(m, align, 0, 0);
    lv_obj_set_style_bg_color(m, lv_color_hex(0x1ABC9C), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(m, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(m, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(m, 0, LV_PART_MAIN);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);

    /* A frame on the outermost pixel. A missing edge means the drawable area is
     * not where we think it is. */
    lv_obj_t *frame = lv_obj_create(screen);
    lv_obj_set_size(frame, LV_PCT(100), LV_PCT(100));
    lv_obj_center(frame);
    lv_obj_set_style_bg_opa(frame, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(frame, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(frame, lv_color_hex(0x404040), LV_PART_MAIN);
    lv_obj_set_style_radius(frame, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(frame, 0, LV_PART_MAIN);

    corner(screen, LV_ALIGN_TOP_LEFT);
    corner(screen, LV_ALIGN_TOP_RIGHT);
    corner(screen, LV_ALIGN_BOTTOM_LEFT);
    corner(screen, LV_ALIGN_BOTTOM_RIGHT);

    /* What the driver believes it is driving, after rotation. */
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    struct display_capabilities caps = {0};
    if (device_is_ready(display)) {
        display_get_capabilities(display, &caps);
    }

    char text[48];
    snprintf(text, sizeof(text), "focus dongle\n%u x %u", caps.x_resolution, caps.y_resolution);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xE0E0E0), LV_PART_MAIN);
    lv_obj_center(label);

    return screen;
}
