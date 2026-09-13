/*
 * Derived from carrefinho/prospector-zmk-module (MIT) — see NOTICE.
 *
 * Changed: the ambient light sensor is gone entirely, along with its sampling
 * thread, its fade curve and its Kconfig option. This module has no such chip
 * to test against, and shipping an untested code path is worse than not
 * offering the feature. What remains is a backlight set at boot and stepped by
 * a key.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(focus_brightness, CONFIG_ZMK_LOG_LEVEL);

#include <focus/brightness.h>

static const struct device *pwm_leds_dev = DEVICE_DT_GET_ONE(pwm_leds);
#define DISP_BL DT_NODE_CHILD_IDX(DT_NODELABEL(disp_bl))

static uint8_t brightness = CONFIG_FOCUS_DONGLE_FIXED_BRIGHTNESS;

static void apply(void) {
    if (led_set_brightness(pwm_leds_dev, DISP_BL, brightness)) {
        LOG_ERR("could not set backlight to %d%%", brightness);
    }
}

void focus_brightness_step(int delta) {
    int next = (int)brightness + delta;

    if (next < FOCUS_BRIGHTNESS_MIN) {
        next = FOCUS_BRIGHTNESS_MIN;
    } else if (next > FOCUS_BRIGHTNESS_MAX) {
        next = FOCUS_BRIGHTNESS_MAX;
    }

    /* Already at the end of the range: nothing to write, and no log line for
     * something the user can see is not happening. */
    if (next == (int)brightness) {
        return;
    }

    brightness = (uint8_t)next;
    apply();
}

static int focus_brightness_init(void) {
    apply();

    return 0;
}

SYS_INIT(focus_brightness_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
