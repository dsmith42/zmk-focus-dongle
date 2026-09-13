#define DT_DRV_COMPAT zmk_behavior_screen_brightness

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <drivers/behavior.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>
#include <focus/brightness.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

/* The real one lives in the shield's brightness.c, which only the dongle build
 * compiles. A split peripheral compiles the same keymap and therefore the same
 * binding, so without this the halves fail to link — and stepping a backlight
 * that does not exist is a no-op anyway. */
__attribute__((weak)) void focus_brightness_step(int delta) { ARG_UNUSED(delta); }

/* One driver, two nodes: the direction and the size of the step are the node's
 * own `step` property rather than a binding parameter.
 *
 * That is why there is no dt-bindings header here. The tree this was derived
 * from passed BRI_UP / BRI_DOWN as ints through one behaviour, which meant a
 * header of magic numbers travelling with the module and a keymap that read
 * `&bri BRI_UP`. Two named nodes read better at the binding site and cost less
 * to ship. */
struct behavior_screen_brightness_config {
    int step;
};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_screen_brightness_config *config = dev->config;

    focus_brightness_step(config->step);

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_screen_brightness_init(const struct device *dev) { return 0; }

static const struct behavior_driver_api behavior_screen_brightness_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

#define SCREEN_BRIGHTNESS_INST(n)                                                                  \
    static const struct behavior_screen_brightness_config config_##n = {                           \
        .step = DT_INST_PROP(n, step),                                                             \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_screen_brightness_init, NULL, NULL, &config_##n,           \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                      \
                            &behavior_screen_brightness_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SCREEN_BRIGHTNESS_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
