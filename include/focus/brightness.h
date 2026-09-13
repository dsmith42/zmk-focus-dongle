/*
 * Display backlight.
 *
 * One value, set at boot from Kconfig and adjustable at runtime. Not
 * persisted: a power cycle returns to the configured brightness, like every
 * other piece of state in this module.
 *
 * There is no auto-brightness. The Prospector's ambient light sensor is an
 * optional add-on breakout rather than a populated part, so this module has
 * never had one to test against — see the README. The panel is meant to be a
 * quiet, glanceable surface, and a screen that changes because a cloud passed
 * is the opposite of that.
 */

#pragma once

/* Percent the backlight may be taken down to. Not zero: an unreadable screen
 * looks identical to a broken one, and the way back is a key you cannot see. */
#define FOCUS_BRIGHTNESS_MIN 5
#define FOCUS_BRIGHTNESS_MAX 100

/* Adjust by delta percent, clamped. Weakly defined in the behaviour that calls
 * it, so a split peripheral — which compiles the same keymap, and therefore the
 * same binding, while having no display — links and does nothing. */
void focus_brightness_step(int delta);
