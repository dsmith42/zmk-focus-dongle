#define DT_DRV_COMPAT zmk_behavior_focus_theme_next

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <drivers/behavior.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>
#include <focus/theme.h>
#include <focus/timer.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

/* The real one lives in src/theme.c, which is display-only: it reads the theme
 * nodes out of devicetree, and those come from the shield overlay that only the
 * dongle build applies.
 *
 * A split peripheral compiles this same keymap and therefore this same binding,
 * so without a fallback the halves would fail to link. Cycling a palette on a
 * board with no screen is a no-op by definition, which is exactly what this
 * returns. Remove it and every half build breaks — the same trap the timer core
 * is shaped around. */
__attribute__((weak)) uint8_t focus_theme_after(uint8_t current) { return current; }

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    struct focus_timer_state state;

    focus_timer_get(&state);
    focus_timer_set_theme(focus_theme_after(state.theme));

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_focus_theme_next_init(const struct device *dev) { return 0; }

static const struct behavior_driver_api behavior_focus_theme_next_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

BEHAVIOR_DT_INST_DEFINE(0, behavior_focus_theme_next_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_focus_theme_next_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
