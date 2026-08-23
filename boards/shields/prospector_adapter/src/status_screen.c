/*
 * Placeholder status screen.
 *
 * ZMK requires a custom status screen once the shield selects
 * ZMK_DISPLAY_STATUS_SCREEN_CUSTOM, so this returns an empty one. Its whole job
 * is to let the shield link, so that flashing proves the panel initialises and
 * the backlight comes on — with nothing drawn that could disguise a failure.
 *
 * Replaced with real content in the next step.
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/display.h>

lv_obj_t *zmk_display_status_screen(void) {
    return lv_obj_create(NULL);
}
