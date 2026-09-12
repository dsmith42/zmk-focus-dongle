/*
 * Focus-block timer state.
 *
 * Stores an absolute deadline rather than a decrementing counter, so remaining
 * time is a pure function of the monotonic uptime clock. Nothing that happens
 * to the display can affect it: hide the widget, stop ticking it, come back
 * twenty minutes later and it still reads correctly, with no drift and no
 * save/restore. Do not reintroduce a ticking countdown.
 *
 * RAM only. A power cycle is a fresh state, deliberately — there is no wall
 * clock on this hardware to restore against.
 */

#pragma once

#include <zephyr/kernel.h>

struct focus_timer_state {
    bool running;
    /* Signed, and NOT clamped at zero. Past the deadline it keeps going
     * negative — a 45 minute block still running at 60 reports -15 here.
     *
     * This is remaining time and only ever that. The display decides what to
     * show: today it counts up in total elapsed, which is
     * `total_ms - remaining_ms` and stays correct as this goes negative. Do not
     * move that arithmetic in here — the core reports one number with one
     * meaning, and presentation is not its business. */
    int64_t remaining_ms;
    int64_t total_ms;
    uint16_t armed_minutes; /* what a start would begin */

    /* Which palette the dial draws this block in — model data, not a colour.
     * The mapping to a hex value happens at the widget, so the view model and
     * its tests never see one.
     *
     * Survives a block, like armed_minutes: the theme is part of what a block
     * WAS, and stopping is when a block gets read. */
    uint8_t theme;
};

/* Select the length a later start will use. Ignored while a block is live,
 * including while overrunning. */
void focus_timer_arm(uint16_t minutes);

/* Start the armed length. Ignored while a block is live, INCLUDING while it is
 * overrunning — stop it first. */
void focus_timer_start_armed(void);

/* Start a block of the given length directly. Ignored while a block is live,
 * including while overrunning. */
void focus_timer_start(uint16_t minutes);

/* Stop and clear. Also the act that ends an overrunning block, which is when
 * its overrun is read. */
void focus_timer_stop(void);

/* Choose the palette. Unlike arm and start, this is allowed WHILE A BLOCK IS
 * RUNNING, and that asymmetry is deliberate: length and elapsed time are the
 * data a stray keypress must not be able to destroy, whereas the theme destroys
 * nothing. Realising twenty minutes in that this block is something else is a
 * real thing that happens, and the correction should be one key. */
void focus_timer_set_theme(uint8_t theme);

/* Current state, recomputed from uptime at call time. */
void focus_timer_get(struct focus_timer_state *out);
