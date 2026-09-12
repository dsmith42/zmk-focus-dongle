/*
 * The status text's view model.
 *
 * The same shape as the dial's (focus/dial.h): a pure function from state to
 * everything the widget needs to draw, with no LVGL, no devicetree and no
 * colours. The widget applies this and decides nothing.
 *
 * In the tree this was derived from, all of this logic lived inline in LVGL
 * callbacks and consequently none of it was ever tested — which is how a UTF-8
 * bug in the layer label survived until it was found by looking at a screen.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <focus/role.h>

/* A Corne is two halves. The dongle itself is bus-powered and has no cell, so
 * every battery on this screen belongs to a peripheral. */
#define FOCUS_BATTERY_MAX 2

/* Sized in BYTES, not characters, and deliberately roomy. Truncating UTF-8
 * mid-sequence leaves a partial codepoint, which renders as a missing glyph
 * rather than as a short name. */
#define FOCUS_LAYER_TEXT_MAX 32

/* Below this the battery numeral turns red. */
#define FOCUS_BATTERY_LOW_PCT 15

/* How the profile indicator is drawn. The view model picks the form; the widget
 * owns which font each form uses.
 *
 * Two forms exist because USB is three letters and will not fit in a circle —
 * so the moment a circled digit was chosen, a text form was needed anyway. That
 * makes a profile index past the baked glyphs free to handle: it falls back to
 * the same text form rather than needing a glyph that is not there. */
enum focus_profile_form {
    FOCUS_PROFILE_CIRCLED, /* ①..⑤ */
    FOCUS_PROFILE_TEXT,    /* "USB", or "B 6" past the circled digits */
};

/* Circled digits baked into the font, and therefore the highest profile index
 * that can be drawn as one. */
#define FOCUS_PROFILE_CIRCLED_MAX 5

struct focus_status_state {
    /* NULL or empty when the layer has no display-name in the keymap. */
    const char *layer_name;
    uint8_t layer_index;
    bool layer_uppercase;

    bool usb;
    uint8_t profile_index; /* 0-based, as ZMK reports it */

    uint8_t battery[FOCUS_BATTERY_MAX];
    uint8_t battery_count;
};

struct focus_status_view {
    char layer[FOCUS_LAYER_TEXT_MAX];
    enum focus_role layer_role;

    /* The circled digit is UTF-8, so this is bytes too: three for a glyph,
     * four for "B 12", and room to spare. */
    char profile[8];
    enum focus_profile_form profile_form;
    enum focus_role profile_role;

    struct {
        char text[4]; /* "0".."100" */
        enum focus_role role;
    } battery[FOCUS_BATTERY_MAX];
    uint8_t battery_count;
};

struct focus_status_view focus_status_view_of(const struct focus_status_state *state);
