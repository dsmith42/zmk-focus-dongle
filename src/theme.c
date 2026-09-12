/*
 * Role -> colour, and the derivations that keep a palette down to one
 * declaration.
 *
 * Every colour on the screen is either here or derived here. Let a derivation
 * leak into a render path and the coupling the roles removed comes straight
 * back, and adding a palette stops being a one-node change.
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#include <focus/theme.h>

/* The resting readout: the dial's numeral and the BLE profile. The derived tree
 * had two barely distinguishable greys here (0x8A8A8A and 0x7B7D93); they are
 * the same decision, so they are the same colour. */
#define THEME_GREY 0x8A8A8A

/* Keyboard state — the layer name and the held modifiers. NOT per palette: the
 * dial says which block this is, and the keyboard's own readouts keep one voice
 * so that switching palette changes one thing on the panel rather than all of
 * it. */
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

/* The palettes, in the order devicetree happened to emit them, each carrying
 * the index a keymap actually binds. Position and index are deliberately not
 * assumed to match: a consumer may add a node of their own, and the numbers
 * already written into their keymap must not shift when they do. */
struct theme {
    uint8_t index;
    uint32_t wedge;
};

#define THEME_ENTRY(node)                                                                          \
    {                                                                                              \
        .index = DT_PROP(node, index),                                                             \
        .wedge = DT_PROP(node, wedge),                                                             \
    },

/* The compatible is spelled out rather than hidden behind a macro:
 * DT_FOREACH_STATUS_OKAY token-pastes its first argument, so an indirect name
 * would never expand and the array would silently come out empty. */
static const struct theme themes[] = {DT_FOREACH_STATUS_OKAY(zmk_focus_theme, THEME_ENTRY)};

BUILD_ASSERT(ARRAY_SIZE(themes) > 0, "no zmk,focus-theme nodes — the dial would have no colour");

#if DT_HAS_CHOSEN(zmk_focus_theme)
#define DEFAULT_INDEX DT_PROP(DT_CHOSEN(zmk_focus_theme), index)
#else
/* No chosen entry: the first declared palette. A dongle with no opinion still
 * draws a dial rather than failing to build. */
#define DEFAULT_INDEX (themes[0].index)
#endif

uint8_t focus_theme_count(void) { return ARRAY_SIZE(themes); }

uint8_t focus_theme_default(void) { return DEFAULT_INDEX; }

/* Linear, over six entries, on a screen that repaints a few times a minute. A
 * lookup table indexed by theme number would be faster and would also have to
 * decide what to do about gaps in the numbering. */
static uint32_t wedge_of(uint8_t theme) {
    for (size_t i = 0; i < ARRAY_SIZE(themes); i++) {
        if (themes[i].index == theme) {
            return themes[i].wedge;
        }
    }

    /* An index nobody declared. Draw the default rather than nothing: a wrong
     * colour is a visible mistake, a black dial looks like a crash. */
    for (size_t i = 0; i < ARRAY_SIZE(themes); i++) {
        if (themes[i].index == DEFAULT_INDEX) {
            return themes[i].wedge;
        }
    }

    return themes[0].wedge;
}

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

uint32_t focus_hex_of(enum focus_role role, uint8_t theme) {
    switch (role) {
    case FOCUS_ROLE_THEME:
        return wedge_of(theme);
    case FOCUS_ROLE_DIM:
        return scale(wedge_of(theme), THEME_ARMED_LEVEL);
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
uint32_t focus_hand_hex(enum focus_role wedge_role, uint8_t theme) {
    uint32_t wedge = wedge_of(theme);

    return wedge_role == FOCUS_ROLE_THEME ? lighten(wedge, THEME_HAND_LIFT) : wedge;
}
