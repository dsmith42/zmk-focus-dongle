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

/* The four modifier glyphs, drawn in GACS order: ⌘ ⌥ ⌃ ⇧.
 *
 * Left and right collapse into one glyph each. The screen answers "is a
 * modifier held", which is what a chord means; which physical key produced it
 * is not information anyone reads off a dongle. */
#define FOCUS_MOD_COUNT 4

enum focus_mod {
    FOCUS_MOD_GUI,
    FOCUS_MOD_ALT,
    FOCUS_MOD_CTRL,
    FOCUS_MOD_SHIFT,
};

/* USB HID modifier bit positions — the specification's own numbering, so the
 * view model needs no ZMK header and the host tests can build the byte by
 * hand. */
#define FOCUS_HID_LCTL (1u << 0)
#define FOCUS_HID_LSFT (1u << 1)
#define FOCUS_HID_LALT (1u << 2)
#define FOCUS_HID_LGUI (1u << 3)
#define FOCUS_HID_RCTL (1u << 4)
#define FOCUS_HID_RSFT (1u << 5)
#define FOCUS_HID_RALT (1u << 6)
#define FOCUS_HID_RGUI (1u << 7)

struct focus_status_state {
    /* NULL or empty when the layer has no display-name in the keymap. */
    const char *layer_name;
    uint8_t layer_index;
    bool layer_uppercase;

    /* HID modifier flags, as ZMK reports the explicit mods. */
    uint8_t mods;

    bool usb;
    uint8_t profile_index; /* 0-based, as ZMK reports it */

    uint8_t battery[FOCUS_BATTERY_MAX];
    uint8_t battery_count;
};

struct focus_status_view {
    char layer[FOCUS_LAYER_TEXT_MAX];
    enum focus_role layer_role;

    /* One role per glyph, GACS order, indexed by enum focus_mod. The glyphs
     * themselves are the widget's business — they are typography, and they
     * never change. What changes is which of them is lit. */
    enum focus_role mods[FOCUS_MOD_COUNT];

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
