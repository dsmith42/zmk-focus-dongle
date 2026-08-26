/*
 * Timer core tests, run on the host.
 *
 * The timer is arithmetic over a monotonic clock, so it can be tested without a
 * target build by controlling that clock directly. A block that takes 45 real
 * minutes to overrun takes no time at all here.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <focus/timer.h>
#include <focus/events/timer_state_changed.h>

#define MIN(m) ((int64_t)(m) * 60 * 1000)

/* --- the fake clock and event sink ------------------------------------- */

static int64_t now_ms;
static int events;

int64_t k_uptime_get(void) { return now_ms; }

int raise_focus_timer_state_changed(struct focus_timer_state_changed ev) {
    (void)ev;
    events++;
    return 0;
}

static void advance(int minutes) { now_ms += MIN(minutes); }

/* --- the harness -------------------------------------------------------- */

static int failures;
static const char *current;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            printf("  FAIL  %s\n        %s:%d  %s\n", current, __FILE__, __LINE__, #cond);         \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

#define CHECK_EQ(actual, expected)                                                                 \
    do {                                                                                           \
        int64_t a_ = (int64_t)(actual), e_ = (int64_t)(expected);                                   \
        if (a_ != e_) {                                                                            \
            printf("  FAIL  %s\n        %s:%d  %s\n        expected %lld, got %lld\n", current,    \
                   __FILE__, __LINE__, #actual, (long long)e_, (long long)a_);                     \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

/* Every test starts from a stopped timer on a fresh clock. The timer keeps
 * static state across tests — as it does across blocks on the device — so this
 * makes each test independent of the order it runs in. */
static void reset(const char *name) {
    current = name;
    now_ms = 0;
    events = 0;
    focus_timer_stop();
    focus_timer_arm(45);
    events = 0;
}

static struct focus_timer_state read(void) {
    struct focus_timer_state s;
    focus_timer_get(&s);
    return s;
}

/* --- tests -------------------------------------------------------------- */

static void test_stopped_is_quiet(void) {
    reset("a stopped timer reports nothing running");
    struct focus_timer_state s = read();
    CHECK(!s.running);
    CHECK_EQ(s.remaining_ms, 0);
    CHECK_EQ(s.total_ms, 0);
}

static void test_start_sets_deadline(void) {
    reset("starting sets remaining and total");
    focus_timer_start(30);
    struct focus_timer_state s = read();
    CHECK(s.running);
    CHECK_EQ(s.remaining_ms, MIN(30));
    CHECK_EQ(s.total_ms, MIN(30));
}

static void test_remaining_follows_the_clock(void) {
    reset("remaining is recomputed from the clock, not decremented");
    focus_timer_start(30);
    advance(10);
    CHECK_EQ(read().remaining_ms, MIN(20));
    advance(19);
    CHECK_EQ(read().remaining_ms, MIN(1));
}

static void test_overrun_goes_negative(void) {
    reset("past the deadline remaining goes negative and keeps going");
    focus_timer_start(45);
    advance(60);
    struct focus_timer_state s = read();
    CHECK(s.running); /* still live, just over */
    CHECK_EQ(s.remaining_ms, -MIN(15));
    advance(60);
    CHECK_EQ(read().remaining_ms, -MIN(75));
}

static void test_display_can_derive_elapsed(void) {
    reset("total elapsed is derivable while overrunning");
    focus_timer_start(45);
    advance(60);
    struct focus_timer_state s = read();
    /* What the dial shows past zero: a 45 minute block still going at 60. */
    CHECK_EQ((s.total_ms - s.remaining_ms) / MIN(1), 60);
}

static void test_running_block_is_locked(void) {
    reset("a running block ignores starts");
    focus_timer_start(30);
    advance(10);
    focus_timer_start(60);
    struct focus_timer_state s = read();
    CHECK_EQ(s.total_ms, MIN(30));   /* still the original block */
    CHECK_EQ(s.remaining_ms, MIN(20));
}

static void test_overrunning_block_is_locked(void) {
    reset("an OVERRUNNING block still ignores starts");
    focus_timer_start(30);
    advance(45); /* 15 minutes over */
    focus_timer_start(60);
    struct focus_timer_state s = read();
    /* Regression guard. The lock used to release at zero, which let a stray
     * start silently reset an overrunning block and discard its overrun — the
     * figure that gets read when the block is logged. */
    CHECK_EQ(s.total_ms, MIN(30));
    CHECK_EQ(s.remaining_ms, -MIN(15));
}

static void test_arm_is_ignored_while_live(void) {
    reset("arming is ignored while a block is live, including overrun");
    focus_timer_start(30);
    focus_timer_arm(15);
    advance(45);
    focus_timer_arm(15);
    focus_timer_stop();
    focus_timer_start_armed();
    /* 45 survived from reset(); neither arm(15) took effect. */
    CHECK_EQ(read().total_ms, MIN(45));
}

static void test_stop_clears(void) {
    reset("stopping clears the block");
    focus_timer_start(30);
    advance(10);
    focus_timer_stop();
    struct focus_timer_state s = read();
    CHECK(!s.running);
    CHECK_EQ(s.remaining_ms, 0);
    CHECK_EQ(s.total_ms, 0);
}

static void test_stop_then_start_switches_length(void) {
    reset("changing length is stop, then start");
    focus_timer_start(30);
    advance(10);
    focus_timer_stop();
    focus_timer_start(60);
    CHECK_EQ(read().total_ms, MIN(60));
}

static void test_zero_stops(void) {
    reset("starting zero minutes stops instead");
    focus_timer_start(30);
    focus_timer_stop();
    focus_timer_start(0);
    CHECK(!read().running);
}

static void test_arm_survives_a_block(void) {
    reset("the armed length survives a block");
    focus_timer_arm(20);
    focus_timer_start_armed();
    CHECK_EQ(read().total_ms, MIN(20));
    advance(20);
    focus_timer_stop();
    focus_timer_start_armed();
    CHECK_EQ(read().total_ms, MIN(20));
    CHECK_EQ(read().armed_minutes, 20);
}

static void test_arm_zero_ignored(void) {
    reset("arming zero is ignored");
    focus_timer_arm(0);
    CHECK_EQ(read().armed_minutes, 45);
}

static void test_events_are_raised(void) {
    reset("state changes raise an event");
    focus_timer_start(30);
    CHECK_EQ(events, 1);
    focus_timer_stop();
    CHECK_EQ(events, 2);
    focus_timer_arm(15);
    CHECK_EQ(events, 3);
}

static void test_ignored_calls_raise_nothing(void) {
    reset("ignored calls raise no event");
    focus_timer_start(30);
    events = 0;
    focus_timer_start(60); /* locked */
    focus_timer_arm(15);   /* locked */
    CHECK_EQ(events, 0);
}

int main(void) {
    test_stopped_is_quiet();
    test_start_sets_deadline();
    test_remaining_follows_the_clock();
    test_overrun_goes_negative();
    test_display_can_derive_elapsed();
    test_running_block_is_locked();
    test_overrunning_block_is_locked();
    test_arm_is_ignored_while_live();
    test_stop_clears();
    test_stop_then_start_switches_length();
    test_zero_stops();
    test_arm_survives_a_block();
    test_arm_zero_ignored();
    test_events_are_raised();
    test_ignored_calls_raise_nothing();

    if (failures) {
        printf("\n%d check(s) failed\n", failures);
        return 1;
    }
    printf("timer core: all checks passed\n");
    return 0;
}
