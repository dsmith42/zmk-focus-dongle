/*
 * Prints the dial's view model for a fixed set of states.
 *
 * The output is committed as tests/dial.snapshot and compared in CI, so a
 * changed constant or a broken angle appears as a readable diff naming the
 * state that moved — a change is either deliberate or it is caught.
 *
 * This is the whole reason the view model is a value rather than a set of LVGL
 * calls. Note what is NOT here: no colours, only roles. An amber build and a
 * teal build produce identical output, so palette work cannot churn the
 * snapshot and a new theme cannot break it.
 */

#include <stdio.h>
#include <stdint.h>

#include <focus/dial.h>

#define MIN(m) ((int64_t)(m) * 60 * 1000)

/* Every role, not only the three the dial emits: the switch is exhaustive on
 * purpose, so adding a role makes this fail to compile rather than quietly
 * printing "?" into a snapshot nobody would re-read. */
static const char *role(enum focus_role r) {
    switch (r) {
    case FOCUS_ROLE_THEME:
        return "theme";
    case FOCUS_ROLE_DIM:
        return "dim";
    case FOCUS_ROLE_GREY:
        return "grey";
    case FOCUS_ROLE_ACCENT:
        return "accent";
    case FOCUS_ROLE_IDLE:
        return "idle";
    case FOCUS_ROLE_OK:
        return "ok";
    case FOCUS_ROLE_LOW:
        return "low";
    }
    return "?";
}

static void emit(const char *label, bool running, int64_t remaining, int64_t total,
                 uint16_t armed) {
    struct focus_timer_state s = {.running = running,
                                  .remaining_ms = remaining,
                                  .total_ms = total,
                                  .armed_minutes = armed};
    struct focus_dial_view v = focus_dial_view_of(&s);

    printf("%s\n", label);
    printf("  numeral      %-4d %s\n", v.numeral, role(v.numeral_role));
    printf("  theme        %d\n", v.theme);
    printf("  hand_deg     %d\n", v.hand_deg);
    printf("  track_deg    %d\n", v.track_deg);
    printf("  wedge_deg    %-4d %s\n", v.wedge_deg, role(v.wedge_role));
    if (v.ring_visible) {
        printf("  ring         %d of %d\n", v.ring_wedge_deg, v.ring_track_deg);
    } else {
        printf("  ring         hidden\n");
    }
    printf("\n");
}

/* One state with a palette selected, to pin that the view model passes the
 * index through and resolves nothing. A red build and a teal build print the
 * same line — the colour is the widget's business. */
static void emit_themed(const char *label, int64_t remaining, int64_t total, uint8_t theme) {
    struct focus_timer_state s = {.running = true,
                                  .remaining_ms = remaining,
                                  .total_ms = total,
                                  .armed_minutes = 45,
                                  .theme = theme};
    struct focus_dial_view v = focus_dial_view_of(&s);

    printf("%s\n", label);
    printf("  numeral      %-4d %s\n", v.numeral, role(v.numeral_role));
    printf("  theme        %d\n", v.theme);
    printf("  wedge_deg    %-4d %s\n", v.wedge_deg, role(v.wedge_role));
    printf("\n");
}

int main(void) {
    emit("armed 45, not started", false, 0, 0, 45);
    emit("armed 90, not started", false, 0, 0, 90);
    emit("running 45 of 45", true, MIN(45), MIN(45), 45);
    emit("running 60 of 60", true, MIN(60), MIN(60), 60);
    emit("running 90 of 90", true, MIN(90), MIN(90), 90);
    emit("running 21 of 45", true, MIN(21), MIN(45), 45);
    emit("running 42 of 60", true, MIN(42), MIN(60), 45);
    emit("running 1 of 45", true, MIN(1), MIN(45), 45);
    emit("running 75 of 90", true, MIN(75), MIN(90), 90);
    emit("running 45 of 90", true, MIN(45), MIN(90), 90);
    emit("exactly zero of 45", true, 0, MIN(45), 45);
    emit("59 seconds over a 45", true, -59 * 1000, MIN(45), 45);
    emit("one minute over a 45", true, -MIN(1), MIN(45), 45);
    emit("overrun 15 past a 45", true, -MIN(15), MIN(45), 45);
    emit("overrun 60 past a 30, hand wraps", true, -MIN(60), MIN(30), 45);
    emit_themed("running 21 of 45, palette 4", MIN(21), MIN(45), 4);
    return 0;
}
