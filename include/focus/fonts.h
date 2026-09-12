/*
 * Fonts baked into this module.
 *
 * One Latin family, DINish, under the SIL Open Font License — see
 * src/fonts/DINish-OFL.txt and NOTICE. The tree this was derived from used three
 * families, two of them commercial and unshippable; collapsing onto DINish is
 * what makes the module redistributable, and a consistent screen is the bonus.
 *
 * One symbol face covers what no Latin text face carries: JuliaMono, for the four
 * modifier glyphs. Also OFL — see src/fonts/JuliaMono-OFL.txt.
 *
 * Each file holds only the glyphs actually drawn. Regenerate with
 * scripts/gen_fonts.sh, which is also the place the glyph sets are declared.
 */

#pragma once

#include <lvgl.h>

/* The dial's minutes numeral. Digits only — the number is never negative. */
extern const lv_font_t DINish_Medium_32;

/* Battery percentages and the BLE profile — digits, plus the three letters of
 * "USB" for when the profile is not in use. */
extern const lv_font_t DINish_Medium_24;

/* The layer label, plus the profile indicator's two text forms. Printable
 * ASCII, because the layer name comes from the consumer's keymap rather than
 * from this module: baking only the characters one keymap happens to use would
 * give everyone else tofu, on a device with no log to explain it. */
extern const lv_font_t DINish_Medium_20;

/* The four modifier glyphs, ⌘⌥⌃⇧. One face carries all four, which is why there
 * is no fallback chain here: see scripts/gen_fonts.sh. */
extern const lv_font_t JuliaMono_Regular_28;
