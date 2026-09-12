/*
 * Role -> colour, and the derivations that keep a theme down to one declaration.
 *
 * Every colour on the screen is either here or derived here. Let a derivation
 * leak into a render path and the coupling the roles removed comes straight
 * back, and multi-theme stops being a one-function change.
 */

#include <focus/theme.h>

#define THEME_WEDGE 0x1ABC9C /* teal — the public default */

/* The resting readout: the dial's numeral and the BLE profile. The derived tree
 * had two barely distinguishable greys here (0x8A8A8A and 0x7B7D93); they are
 * the same decision, so they are now the same colour. */
#define THEME_GREY 0x8A8A8A

/* Keyboard state — the layer name and the held modifiers. Hand picked per theme
 * rather than derived: the amber theme pairs its wedge with a pale BLUE accent,
 * so no single rule produces both palettes.
 *
 * IDLE is a dark tint of the same family rather than the background colour,
 * because an unheld modifier is PRESENT and not held — the four glyphs keep
 * their places so a chord is read by what lit up, not by what appeared. */
#define THEME_ACCENT 0xC8EFE6
#define THEME_IDLE 0x2F5A52

/* A health readout and its alarm. Low is text colour only — no badge, no
 * background, no flash. Quiet by design; the trade is that it is easy to miss
 * unless you are already looking at that corner. */
#define THEME_OK 0x54806C
#define THEME_LOW 0xFF3B30

/* Percent toward white, and percent of the wedge. */
#define THEME_HAND_LIFT 40
#define THEME_ARMED_LEVEL 45

static uint32_t lighten(uint32_t c, int pct) {
    uint32_t r = (c >> 16) & 0xff, g = (c >> 8) & 0xff, b = c & 0xff;

    r += ((255 - r) * pct) / 100;
    g += ((255 - g) * pct) / 100;
    b += ((255 - b) * pct) / 100;

    return (r << 16) | (g << 8) | b;
}

static uint32_t scale(uint32_t c, int pct) {
    uint32_t r = (((c >> 16) & 0xff) * pct) / 100;
    uint32_t g = (((c >> 8) & 0xff) * pct) / 100;
    uint32_t b = ((c & 0xff) * pct) / 100;

    return (r << 16) | (g << 8) | b;
}

uint32_t focus_hex_of(enum focus_role role) {
    switch (role) {
    case FOCUS_ROLE_THEME:
        return THEME_WEDGE;
    case FOCUS_ROLE_DIM:
        return scale(THEME_WEDGE, THEME_ARMED_LEVEL);
    case FOCUS_ROLE_ACCENT:
        return THEME_ACCENT;
    case FOCUS_ROLE_IDLE:
        return THEME_IDLE;
    case FOCUS_ROLE_OK:
        return THEME_OK;
    case FOCUS_ROLE_LOW:
        return THEME_LOW;
    case FOCUS_ROLE_GREY:
    default:
        return THEME_GREY;
    }
}

/*
 *            wedge                     hand
 *   armed    scale(wedge, 45%)   ->    wedge            (luma  67 -> 151)
 *   running  wedge               ->    lighten(wedge)   (luma 151 -> 192)
 *
 * So starting a block lifts the whole assembly by one step rather than changing
 * one element, which is what the start gesture should look like.
 *
 * Two earlier versions were wrong in opposite directions. Making the armed hand
 * equal its own wedge hid it — and the armed state is precisely when the hand
 * matters, being the only thing showing where the block will end. Leaving it at
 * full running brightness put it above even the ACTIVE wedge, so nothing read as
 * inactive. One step up from whatever is underneath solves both.
 *
 * Note the armed hand and the running wedge are the same value. Different shapes
 * and never on screen in the same state; confirmed acceptable on hardware.
 */
uint32_t focus_hand_hex(enum focus_role wedge_role) {
    return wedge_role == FOCUS_ROLE_THEME ? lighten(THEME_WEDGE, THEME_HAND_LIFT) : THEME_WEDGE;
}
