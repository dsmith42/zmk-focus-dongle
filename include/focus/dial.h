/*
 * The dial's view model.
 *
 * A pure function from timer state to everything the widget needs to draw. No
 * LVGL, no devicetree, no colours — the widget applies this and decides
 * nothing, which is what makes both host-testable.
 *
 * Colours are ROLES, not values. The theme mapping happens at the widget, so
 * this layer stays theme-independent: an amber build and a teal build produce
 * identical view models, and adding a theme cannot break a test.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <focus/timer.h>

/* The disc is an absolute 60 minute face. Longer blocks spill onto the overflow
 * ring, so nothing wraps and nothing jumps. */
#define FOCUS_DIAL_FACE_MINUTES 60

/* One disc plus one ring, so two hours is the ceiling. */
#define FOCUS_DIAL_MAX_MINUTES 120

enum focus_role {
    FOCUS_ROLE_THEME, /* the wedge colour, at full strength */
    FOCUS_ROLE_DIM,   /* armed but not started */
    FOCUS_ROLE_GREY,  /* the resting colour for the numeral */
};

struct focus_dial_view {
    /* The number to print. Minutes remaining while a block runs, rounded UP so
     * it never reads finished with 59 seconds left. Past zero it is total
     * elapsed instead, rounded DOWN, so 45:00 through 45:59 all read 45. */
    int numeral;
    enum focus_role numeral_role;

    /* Degrees clockwise from twelve. */
    int hand_deg;

    /* The block's length on the disc, and how much of it is left. */
    int track_deg;
    int wedge_deg;

    /* The overflow ring, for blocks longer than the face. */
    bool ring_visible;
    int ring_track_deg;
    int ring_wedge_deg;

    enum focus_role wedge_role;
};

struct focus_dial_view focus_dial_view_of(const struct focus_timer_state *state);
