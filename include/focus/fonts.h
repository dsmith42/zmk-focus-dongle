/*
 * Fonts baked into this module.
 *
 * One Latin family, DINish, under the SIL Open Font License — see
 * src/fonts/DINish-OFL.txt and NOTICE. The tree this was derived from used three
 * families, two of them commercial and unshippable; collapsing onto DINish is
 * what makes the module redistributable, and a consistent screen is the bonus.
 *
 * Each file holds only the glyphs actually drawn. Regenerate with
 * scripts/gen_dinish.sh, which is also the place the glyph sets are declared.
 */

#pragma once

#include <lvgl.h>

/* The dial's minutes numeral. Digits only — the number is never negative. */
extern const lv_font_t DINish_Medium_32;
