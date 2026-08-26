/*
 * Dial view-model tests.
 *
 * The view model is a pure function from timer state to what should be drawn,
 * so it can be exercised without LVGL, a panel, or a theme. What it cannot tell
 * us is whether the result *looks* right — that is a flash.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <focus/dial.h>

#define MIN(m) ((int64_t)(m) * 60 * 1000)
#define SEC(s) ((int64_t)(s) * 1000)

static int failures;
static const char *current;

#define CHECK_EQ(actual, expected)                                                                 \
    do {                                                                                           \
        int64_t a_ = (int64_t)(actual), e_ = (int64_t)(expected);                                  \
        if (a_ != e_) {                                                                            \
            printf("  FAIL  %s\n        %s:%d  %s\n        expected %lld, got %lld\n", current,    \
                   __FILE__, __LINE__, #actual, (long long)e_, (long long)a_);                     \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

static struct focus_dial_view view(const char *name, bool running, int64_t remaining,
                                   int64_t total, uint16_t armed) {
    current = name;
    struct focus_timer_state s = {.running = running,
                                  .remaining_ms = remaining,
                                  .total_ms = total,
                                  .armed_minutes = armed};
    return focus_dial_view_of(&s);
}

/* --- armed --------------------------------------------------------------- */

static void test_armed_previews_at_full_extent(void) {
    struct focus_dial_view v = view("armed previews the length, dimmed", false, 0, 0, 45);
    CHECK_EQ(v.numeral, 45);
    CHECK_EQ(v.numeral_role, FOCUS_ROLE_DIM);
    CHECK_EQ(v.wedge_role, FOCUS_ROLE_DIM);
    CHECK_EQ(v.wedge_deg, 270);
    CHECK_EQ(v.track_deg, 270);
    CHECK_EQ(v.ring_visible, false);
}

static void test_armed_beyond_the_face_shows_the_ring(void) {
    struct focus_dial_view v = view("an armed 90 spills onto the ring", false, 0, 0, 90);
    CHECK_EQ(v.numeral, 90);
    CHECK_EQ(v.wedge_deg, 360);
    CHECK_EQ(v.ring_visible, true);
    CHECK_EQ(v.ring_wedge_deg, 180);
}

/* --- running ------------------------------------------------------------- */

static void test_running_counts_down_in_grey(void) {
    struct focus_dial_view v = view("running counts down, wedge carries the colour", true,
                                    MIN(21), MIN(45), 45);
    CHECK_EQ(v.numeral, 21);
    CHECK_EQ(v.numeral_role, FOCUS_ROLE_GREY);
    CHECK_EQ(v.wedge_role, FOCUS_ROLE_THEME);
    CHECK_EQ(v.track_deg, 270);
    CHECK_EQ(v.wedge_deg, 126);
    CHECK_EQ(v.hand_deg, 126);
}

static void test_minutes_round_up(void) {
    /* Never read finished with 59 seconds left. */
    struct focus_dial_view v = view("a part minute rounds up", true, SEC(1), MIN(45), 45);
    CHECK_EQ(v.numeral, 1);
    v = view("59 seconds still reads 1", true, SEC(59), MIN(45), 45);
    CHECK_EQ(v.numeral, 1);
}

static void test_long_block_fills_disc_then_ring(void) {
    struct focus_dial_view v = view("a 90 minute block uses disc and ring", true, MIN(75),
                                    MIN(90), 90);
    CHECK_EQ(v.numeral, 75);
    CHECK_EQ(v.wedge_deg, 360);       /* disc full */
    CHECK_EQ(v.ring_visible, true);
    CHECK_EQ(v.ring_wedge_deg, 90);   /* 15 minutes of the second hour */
}

/* --- overrun ------------------------------------------------------------- */

static void test_overrun_counts_up_in_theme(void) {
    /* A 45 minute block still going at 60. */
    struct focus_dial_view v = view("overrun counts up in total elapsed", true, -MIN(15),
                                    MIN(45), 45);
    CHECK_EQ(v.numeral, 60);
    CHECK_EQ(v.numeral_role, FOCUS_ROLE_THEME);
    CHECK_EQ(v.wedge_deg, 0); /* spent */
}

static void test_countdown_rests_at_zero_for_a_minute(void) {
    /* The countdown reaches zero and stays there until a whole minute is over,
     * so it actually completes rather than jumping from 1 to the elapsed
     * total. */
    struct focus_dial_view v = view("exactly zero reads 0", true, 0, MIN(45), 45);
    CHECK_EQ(v.numeral, 0);
    CHECK_EQ(v.numeral_role, FOCUS_ROLE_THEME);
    v = view("still 0 at 59 seconds over", true, -SEC(59), MIN(45), 45);
    CHECK_EQ(v.numeral, 0);
    v = view("counting up begins at a minute over", true, -MIN(1), MIN(45), 45);
    CHECK_EQ(v.numeral, 46);
}

static void test_overrun_minute_boundary_is_exact(void) {
    /* Regression guard. The old round-up formula truncated toward zero once ms
     * went negative, making a 15 minute overrun read as 14. */
    struct focus_dial_view v = view("a 15 minute overrun is 15, not 14", true, -MIN(15),
                                    MIN(30), 45);
    CHECK_EQ(v.numeral, 45); /* 30 + 15 */
    CHECK_EQ(v.hand_deg, 270);
}

static void test_hand_sweeps_continuously_through_zero(void) {
    /* The hand moves in whole minutes, so the interesting property is that the
     * sequence keeps going the same way across the boundary rather than
     * jumping: 6 degrees, twelve, 354 degrees. */
    struct focus_dial_view a = view("a minute left", true, SEC(30), MIN(45), 45);
    struct focus_dial_view b = view("at the boundary", true, -SEC(30), MIN(45), 45);
    struct focus_dial_view c = view("a minute over", true, -SEC(90), MIN(45), 45);
    CHECK_EQ(a.hand_deg, 6);   /* 30s left rounds up to 1 minute */
    CHECK_EQ(b.hand_deg, 0);   /* 30s over is still zero whole minutes over */
    CHECK_EQ(c.hand_deg, 354); /* 90s over is one, continuing anticlockwise */
}

static void test_hand_wraps_past_a_full_hour_over(void) {
    struct focus_dial_view v = view("past an hour over the hand wraps", true, -MIN(60),
                                    MIN(30), 45);
    CHECK_EQ(v.hand_deg, 0);  /* back at twelve */
    CHECK_EQ(v.numeral, 90);  /* and the numeral is what disambiguates */
}

int main(void) {
    test_armed_previews_at_full_extent();
    test_armed_beyond_the_face_shows_the_ring();
    test_running_counts_down_in_grey();
    test_minutes_round_up();
    test_long_block_fills_disc_then_ring();
    test_overrun_counts_up_in_theme();
    test_countdown_rests_at_zero_for_a_minute();
    test_overrun_minute_boundary_is_exact();
    test_hand_sweeps_continuously_through_zero();
    test_hand_wraps_past_a_full_hour_over();

    if (failures) {
        printf("\n%d check(s) failed\n", failures);
        return 1;
    }
    printf("dial view model: all checks passed\n");
    return 0;
}
