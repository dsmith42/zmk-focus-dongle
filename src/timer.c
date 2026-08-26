#include <zephyr/kernel.h>

#include <focus/timer.h>
#include <focus/events/timer_state_changed.h>
#include <zmk/event_manager.h>

static int64_t deadline_ms;
static int64_t total_ms;
static bool running;

/* Survives a block, so the common case is start-without-choosing. 45 is the
 * default because it is the most generally useful block length. Not persisted:
 * a power cycle is a fresh state, like everything else here. */
static uint16_t armed_minutes = 45;

/* A block stays locked past its deadline, not just up to it. Overrun is data —
 * it is read when the block is logged — so it must survive until it is
 * deliberately dismissed. Releasing the lock at zero would let a stray start
 * silently reset the counter and take the overrun figure with it.
 *
 * The consequence is intended: after a block expires you must stop before
 * starting another. Stop is the act that ends the block, and it is the moment
 * the overrun gets written down. */
static bool block_is_running(void) { return running; }

static void raise_changed(void) {
    raise_focus_timer_state_changed((struct focus_timer_state_changed){.running = running});
}

void focus_timer_start(uint16_t minutes) {
    if (minutes == 0) {
        focus_timer_stop();
        return;
    }

    /* A running block is locked: starts are ignored until it is stopped
     * deliberately or reaches zero. Elapsed time is the one piece of state the
     * system cares about, and a stray keypress must not be able to discard it.
     * Switching length mid-block is therefore two deliberate acts — stop, then
     * start — which is the intent. */
    if (block_is_running()) {
        return;
    }

    total_ms = (int64_t)minutes * 60 * 1000;
    deadline_ms = k_uptime_get() + total_ms;
    running = true;

    raise_changed();
}

void focus_timer_stop(void) {
    running = false;
    total_ms = 0;
    deadline_ms = 0;

    raise_changed();
}

void focus_timer_arm(uint16_t minutes) {
    if (block_is_running() || minutes == 0) {
        return;
    }

    armed_minutes = minutes;
    raise_changed();
}

void focus_timer_start_armed(void) { focus_timer_start(armed_minutes); }

void focus_timer_get(struct focus_timer_state *out) {
    out->armed_minutes = armed_minutes;
    out->running = running;
    out->total_ms = total_ms;

    if (!running) {
        out->remaining_ms = 0;
        return;
    }

    /* Deliberately unclamped. Once past the deadline this goes negative and
     * keeps going, which is how overrun exists as data at all.
     *
     * `running` stays true until stopped, so an expired block is
     * distinguishable from a stopped one. The display needs both facts: a
     * finished-but-unstopped block is drawn differently from one never
     * started. */
    out->remaining_ms = deadline_ms - k_uptime_get();
}
