/*
 * Colour roles.
 *
 * Every view model in this module emits a ROLE rather than a colour value, and
 * the mapping to hex happens in one place (focus/theme.h). Three reasons, all
 * of which paid off once there were two view models rather than one:
 *
 *   - the pure layer stays theme-independent, so its tests need no theme header
 *   - a teal build and an amber build produce identical view models, because the
 *     decision is the same and only the mapping differs
 *   - adding a theme cannot break a test, which it otherwise would
 *
 * Roles are named for what they MEAN, not for the element that uses them. One
 * role per widget would be no abstraction at all — it is the reason the derived
 * tree ended up with three separate near-identical greys for the numeral, the
 * profile and the battery.
 */

#pragma once

enum focus_role {
    FOCUS_ROLE_THEME,  /* carries the block colour, at full strength */
    FOCUS_ROLE_DIM,    /* armed but not started */
    FOCUS_ROLE_GREY,   /* a resting readout: consulted, never watched */
    FOCUS_ROLE_ACCENT, /* keyboard state, live — the layer, a held modifier */
    FOCUS_ROLE_IDLE,   /* ...and the same thing not live. Present, not absent. */
    FOCUS_ROLE_OK,     /* a health readout inside its normal range */
    FOCUS_ROLE_LOW,    /* ...and outside it. The only alarm colour here. */
};
