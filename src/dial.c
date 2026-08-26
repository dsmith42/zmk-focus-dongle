#include <focus/dial.h>

#define MS_PER_MIN 60000

/* Degrees of dial per minute of the face. */
static int deg_of(int minutes) {
    if (minutes <= 0) {
        return 0;
    }
    if (minutes >= FOCUS_DIAL_FACE_MINUTES) {
        return 360;
    }
    return (minutes * 360) / FOCUS_DIAL_FACE_MINUTES;
}

/* Round up, and correctly for negatives.
 *
 * The obvious `(ms + MS_PER_MIN - 1) / MS_PER_MIN` is wrong once ms goes
 * negative: C truncates division toward zero, so -900000 (fifteen minutes over)
 * comes out as -14. That would have been an off-by-one from the first frame of
 * every overrun. */
static int minutes_up(int64_t ms) {
    if (ms >= 0) {
        return (int)((ms + MS_PER_MIN - 1) / MS_PER_MIN);
    }
    return (int)(-((-ms) / MS_PER_MIN));
}

/* Round down, for a count that is going up. 45:00 through 45:59 all read 45. */
static int minutes_down(int64_t ms) {
    if (ms >= 0) {
        return (int)(ms / MS_PER_MIN);
    }
    return (int)(-(((-ms) + MS_PER_MIN - 1) / MS_PER_MIN));
}

/* Where the hand points, given minutes left on the face.
 *
 * Kept in [0,60) so the hand sweeps continuously through twelve rather than
 * jumping: one minute over reads as 59 on the face, not as -1. Past a full hour
 * of overrun it wraps, and the numeral is what disambiguates that. */
static int hand_deg_of(int face_minutes) {
    int m = face_minutes % FOCUS_DIAL_FACE_MINUTES;
    if (m < 0) {
        m += FOCUS_DIAL_FACE_MINUTES;
    }
    return deg_of(m);
}

struct focus_dial_view focus_dial_view_of(const struct focus_timer_state *state) {
    struct focus_dial_view v = {0};

    /* Not running: preview the armed length at full extent, dimmed. Choosing a
     * length is visible rather than a hidden mode. */
    if (!state->running) {
        int armed = (int)state->armed_minutes;
        if (armed > FOCUS_DIAL_MAX_MINUTES) {
            armed = FOCUS_DIAL_MAX_MINUTES;
        }

        v.numeral = armed;
        v.numeral_role = FOCUS_ROLE_DIM;
        v.wedge_role = FOCUS_ROLE_DIM;
        v.track_deg = deg_of(armed);
        v.wedge_deg = deg_of(armed);
        v.hand_deg = hand_deg_of(armed);
        v.ring_visible = armed > FOCUS_DIAL_FACE_MINUTES;
        v.ring_track_deg = deg_of(armed - FOCUS_DIAL_FACE_MINUTES);
        v.ring_wedge_deg = v.ring_track_deg;
        return v;
    }

    int total_min = (int)(state->total_ms / MS_PER_MIN);
    if (total_min > FOCUS_DIAL_MAX_MINUTES) {
        total_min = FOCUS_DIAL_MAX_MINUTES;
    }

    int left_min = minutes_up(state->remaining_ms);

    v.wedge_role = FOCUS_ROLE_THEME;
    v.track_deg = deg_of(total_min);
    v.ring_visible = total_min > FOCUS_DIAL_FACE_MINUTES;
    v.ring_track_deg = deg_of(total_min - FOCUS_DIAL_FACE_MINUTES);

    if (left_min > 0) {
        /* Running. The numeral counts down in grey; the wedge carries the
         * theme colour. */
        if (left_min > total_min) {
            left_min = total_min;
        }
        v.numeral = left_min;
        v.numeral_role = FOCUS_ROLE_GREY;
        v.wedge_deg = deg_of(left_min);
        v.ring_wedge_deg = deg_of(left_min - FOCUS_DIAL_FACE_MINUTES);
        v.hand_deg = hand_deg_of(left_min);
        return v;
    }

    /* Overrun. The wedge is spent, and the theme colour hands over to the
     * numeral — which switches from counting down to counting up in total
     * elapsed. The hand keeps sweeping, so there are two channels saying the
     * same thing rather than colour alone.
     *
     * The countdown rests at zero for the first minute over. Without it the
     * numeral would never actually reach zero — it would read 2, 1, then jump
     * straight to the elapsed total — and the countdown completing is worth
     * seeing. Counting up starts once there is a whole minute to count. */
    int64_t over_ms = -state->remaining_ms;
    v.numeral = (over_ms < MS_PER_MIN) ? 0 : minutes_down(state->total_ms - state->remaining_ms);
    v.numeral_role = FOCUS_ROLE_THEME;
    v.wedge_deg = 0;
    v.ring_wedge_deg = 0;
    v.hand_deg = hand_deg_of(left_min);
    return v;
}
