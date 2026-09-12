/*
 * Status view-model tests.
 *
 * Same bargain as the dial's: the view model is a pure function from keyboard
 * state to what should be drawn, so every edge case below is reachable from the
 * host. What it cannot say is whether the result LOOKS right — that is a flash.
 *
 * These are the cases that actually bite. The tree this was derived from had
 * all of this inline in an LVGL callback, and its layer label carried a real
 * UTF-8 bug for as long as it existed because nothing here was reachable.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <focus/status.h>

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

#define CHECK_STR(actual, expected)                                                                \
    do {                                                                                           \
        const char *a_ = (actual), *e_ = (expected);                                               \
        if (strcmp(a_, e_) != 0) {                                                                 \
            printf("  FAIL  %s\n        %s:%d  %s\n        expected \"%s\", got \"%s\"\n",         \
                   current, __FILE__, __LINE__, #actual, e_, a_);                                  \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

static struct focus_status_view layer_view(const char *name, const char *layer_name,
                                           uint8_t index, bool uppercase) {
    current = name;
    struct focus_status_state s = {
        .layer_name = layer_name, .layer_index = index, .layer_uppercase = uppercase};
    return focus_status_view_of(&s);
}

static struct focus_status_view profile_view(const char *name, bool usb, uint8_t index) {
    current = name;
    struct focus_status_state s = {.usb = usb, .profile_index = index};
    return focus_status_view_of(&s);
}

static struct focus_status_view battery_view(const char *name, uint8_t a, uint8_t b) {
    current = name;
    struct focus_status_state s = {.battery = {a, b}, .battery_count = 2};
    return focus_status_view_of(&s);
}

/* --- layer --------------------------------------------------------------- */

static void test_layer_uses_the_keymap_name(void) {
    struct focus_status_view v = layer_view("a named layer shows its name", "base", 0, false);
    CHECK_STR(v.layer, "base");
    CHECK_EQ(v.layer_role, FOCUS_ROLE_ACCENT);
}

static void test_layer_uppercases_when_asked(void) {
    struct focus_status_view v = layer_view("uppercase is applied", "base", 0, true);
    CHECK_STR(v.layer, "BASE");
}

static void test_unnamed_layer_falls_back_to_its_index(void) {
    struct focus_status_view a = layer_view("an unnamed layer shows L<n>", NULL, 3, false);
    struct focus_status_view b = layer_view("an empty name is the same case", "", 3, false);
    CHECK_STR(a.layer, "L3");
    CHECK_STR(b.layer, "L3");
}

/* The bug that survived the entire life of the derived tree. A bytewise
 * toupper() over UTF-8 mangles the lead and continuation bytes of anything
 * outside ASCII, and what reaches the panel is a missing glyph with no log to
 * explain it. Japanese layer names are a later rung, but the rule has to hold
 * from the first build or the same bug comes back with it. */
static void test_uppercase_leaves_utf8_alone(void) {
    struct focus_status_view v = layer_view("uppercase skips non-ASCII bytes", "基本", 0, true);
    CHECK_STR(v.layer, "基本");
}

static void test_a_long_name_is_truncated_not_overrun(void) {
    struct focus_status_view v =
        layer_view("an over-long name is cut, not overrun",
                   "a-layer-name-far-longer-than-the-buffer-allows", 0, false);
    CHECK_EQ(strlen(v.layer), FOCUS_LAYER_TEXT_MAX - 1);
}

/* --- profile ------------------------------------------------------------- */

static void test_profile_is_a_circled_digit_one_based(void) {
    struct focus_status_view v = profile_view("profile 0 draws as ①", false, 0);
    CHECK_STR(v.profile, "①");
    CHECK_EQ(v.profile_form, FOCUS_PROFILE_CIRCLED);
    CHECK_EQ(v.profile_role, FOCUS_ROLE_GREY);
}

static void test_last_circled_profile(void) {
    struct focus_status_view v = profile_view("profile 4 draws as ⑤", false, 4);
    CHECK_STR(v.profile, "⑤");
    CHECK_EQ(v.profile_form, FOCUS_PROFILE_CIRCLED);
}

static void test_usb_wins_over_the_profile(void) {
    struct focus_status_view v = profile_view("USB beats a selected profile", true, 2);
    CHECK_STR(v.profile, "USB");
    CHECK_EQ(v.profile_form, FOCUS_PROFILE_TEXT);
}

/* ZMK derives its profile count from CONFIG_BT_MAX_PAIRED, so a config with
 * more profiles than there are baked glyphs is legal. It must degrade to text
 * rather than to a box. */
static void test_profile_past_the_glyphs_falls_back_to_text(void) {
    struct focus_status_view v = profile_view("a sixth profile falls back to text", false, 5);
    CHECK_STR(v.profile, "B 6");
    CHECK_EQ(v.profile_form, FOCUS_PROFILE_TEXT);
}

/* --- battery ------------------------------------------------------------- */

static void test_battery_levels_are_plain_numbers(void) {
    struct focus_status_view v = battery_view("both halves report", 100, 61);
    CHECK_EQ(v.battery_count, 2);
    CHECK_STR(v.battery[0].text, "100");
    CHECK_STR(v.battery[1].text, "61");
    CHECK_EQ(v.battery[0].role, FOCUS_ROLE_OK);
}

static void test_the_low_threshold_is_exclusive(void) {
    struct focus_status_view v = battery_view("15 is fine, 14 is low", 15, 14);
    CHECK_EQ(v.battery[0].role, FOCUS_ROLE_OK);
    CHECK_EQ(v.battery[1].role, FOCUS_ROLE_LOW);
}

/* A peripheral that has never reported reads as zero rather than as unknown,
 * which is indistinguishable from a flat one. Showing it as low is the safe
 * direction: the false alarm is cheap, the missed warning is not. */
static void test_zero_reads_as_low(void) {
    struct focus_status_view v = battery_view("zero is low", 0, 0);
    CHECK_STR(v.battery[0].text, "0");
    CHECK_EQ(v.battery[0].role, FOCUS_ROLE_LOW);
}

int main(void) {
    test_layer_uses_the_keymap_name();
    test_layer_uppercases_when_asked();
    test_unnamed_layer_falls_back_to_its_index();
    test_uppercase_leaves_utf8_alone();
    test_a_long_name_is_truncated_not_overrun();

    test_profile_is_a_circled_digit_one_based();
    test_last_circled_profile();
    test_usb_wins_over_the_profile();
    test_profile_past_the_glyphs_falls_back_to_text();

    test_battery_levels_are_plain_numbers();
    test_the_low_threshold_is_exclusive();
    test_zero_reads_as_low();

    if (failures) {
        printf("\n%d check(s) failed\n", failures);
        return 1;
    }
    printf("status view model: all checks passed\n");
    return 0;
}
