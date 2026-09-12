/*
 * The single seam between roles and colour values.
 *
 * The body of focus_hex_of() was a switch returning constants while one palette
 * shipped; it now indexes a table built from devicetree. Nothing else in either
 * widget changed when that happened, which is the whole reason every colour was
 * routed through here from the first rung.
 *
 * ONLY THE DIAL IS THEMED. The layer name, the modifier row, the battery
 * colours and the structural greys are the same in every palette — they are
 * keyboard state and furniture, not the block. So selecting a theme repaints
 * one element rather than the screen.
 */

#pragma once

#include <stdint.h>

#include <focus/role.h>

/* Structural dial furniture: three even steps, face (the whole hour) < block
 * (the length chosen) < wedge (what remains). These never appear in a view
 * model — they are construction-time constants, not decisions — so they are
 * plain values rather than roles. */
#define FOCUS_COLOR_FACE 0x242424
#define FOCUS_COLOR_BLOCK 0x3A3A3A
#define FOCUS_COLOR_TICK 0x9A9A9A
#define FOCUS_COLOR_HUB 0xD8D4CC

/* For call sites whose roles never consult the theme — everything the status
 * text draws. Passing this says "no palette is involved here" rather than
 * reaching for an arbitrary index. */
#define FOCUS_THEME_ANY 0

/* How many palettes devicetree declared. */
uint8_t focus_theme_count(void);

/* The index a block starts in, from the chosen node. */
uint8_t focus_theme_default(void);

/* Map a role a view model emitted to the colour that draws it.
 *
 * `theme` is consulted only for the roles that carry the block's colour —
 * FOCUS_ROLE_THEME and FOCUS_ROLE_DIM. Every other role is the same in every
 * palette, deliberately; see the note at the top. An index with no theme behind
 * it falls back to the default rather than drawing nothing. */
uint32_t focus_hex_of(enum focus_role role, uint8_t theme);

/* The dial's hand, which is always ONE STEP brighter than the wedge it points
 * over. Derived rather than declared, so a palette still only names its wedge. */
uint32_t focus_hand_hex(enum focus_role wedge_role, uint8_t theme);
