/*
 * The single seam between roles and colour values.
 *
 * ONE THEME, HARDCODED. Themes are a later rung, and the whole point of routing
 * every colour through here is that the later rung is a change to one function
 * body: today it returns constants, later it indexes a table built from
 * devicetree, and no widget changes either way.
 *
 * ⛔ Not built yet, deliberately: no zmk,focus-theme binding, no theme nodes in
 * the overlay, no chosen-node fallback. That is real plumbing with real cost.
 * Declining to SCATTER the colours is not the same as building it early.
 */

#pragma once

#include <stdint.h>

#include <focus/role.h>

/* Structural dial furniture: three even steps, face (the whole hour) < block
 * (the length chosen) < wedge (what remains). These never appear in a view
 * model — they are construction-time constants, not decisions — so they are
 * plain values rather than roles. They live here so that "the colours are in
 * one file" stays true. */
#define FOCUS_COLOR_FACE 0x242424
#define FOCUS_COLOR_BLOCK 0x3A3A3A
#define FOCUS_COLOR_TICK 0x9A9A9A
#define FOCUS_COLOR_HUB 0xD8D4CC

/* Map a role a view model emitted to the colour that draws it. */
uint32_t focus_hex_of(enum focus_role role);

/* The dial's hand, which is always ONE STEP brighter than the wedge it points
 * over. Derived rather than declared, so a theme still only names its wedge. */
uint32_t focus_hand_hex(enum focus_role wedge_role);
