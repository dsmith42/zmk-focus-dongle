#!/usr/bin/env bash
#
# Regenerate the Latin glyph subsets used by the display.
#
# The point of this script is that we never ship a whole face. A full DINish cut
# is ~100 KB of flash per size; the glyphs actually drawn are a handful. Baking
# only those keeps each font in the low single-digit KB, and makes it obvious in
# review when the set changes.
#
# DINish is the one Latin family on this screen, deliberately. The tree this was
# derived from used three (DINish, Foundry Gridnik, FRAC) of which two were
# commercial and could not be redistributed. Collapsing onto DINish is what makes
# the module shareable, and it is a better-looking screen for being consistent.
#
# Requires node (for npx) and network. The TTF is downloaded rather than vendored,
# both to keep the repo small and to make the OFL provenance obvious.

set -euo pipefail

BPP=4

# DINish: SIL Open Font License 1.1. https://github.com/playbeing/dinish
BASE_URL="https://raw.githubusercontent.com/playbeing/dinish/master/fonts/ttf"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out_dir="$repo_root/src/fonts"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

# generate <out-name> <cut-dir> <ttf> <size> <symbols> <what-it-is>
generate() {
    local out_name="$1" cut="$2" ttf="$3" size="$4" symbols="$5" what="$6"

    if [ ! -f "$work/$ttf" ]; then
        echo "Fetching $ttf..."
        curl -sSL --fail -o "$work/$ttf" "$BASE_URL/$cut/$ttf"
    fi

    echo "Converting ${what}: ${#symbols} glyphs at ${size}px, ${BPP}bpp..."
    npx -y lv_font_conv@1.5.2 \
        --font "$work/$ttf" \
        --bpp "$BPP" \
        --size "$size" \
        --no-compress \
        --format lvgl \
        --symbols "$symbols" \
        -o "$out_dir/${out_name}.c"

    python3 "$repo_root/scripts/fixup_font.py" "$out_dir/${out_name}.c"

    # lv_font_conv records the exact argv it was invoked with in a comment at the
    # top of the output. Those are absolute paths into a temp dir and someone's
    # home directory, so the file would differ on every machine and leak the path.
    # Rewrite them to a stable form, which makes regeneration a byte-for-byte
    # no-op and keeps the diff honest about whether the glyphs actually changed.
    sed -i.bak \
        -e "s| --font [^ ]*/${ttf}| --font ttf/${ttf}|" \
        -e "s| -o [^ ]*/${out_name}.c| -o ${out_name}.c|" \
        "$out_dir/${out_name}.c"
    rm -f "$out_dir/${out_name}.c.bak"

    echo "  wrote ${out_name}.c ($(wc -c < "$out_dir/${out_name}.c") bytes of C)"
}

# The dial's minutes numeral. Digits only: the view model documents this number
# as never negative -- it counts remaining down to zero, then total elapsed up --
# so there is no sign to draw. If that ever changes, add a hyphen here.
#
# Size matches the FR_Medium_32 it replaces. DINish and FRAC do not share a cap
# height, so this is a starting point to judge on hardware, not a match.
generate "DINish_Medium_32" "DINish" "DINish-Medium.ttf" 32 "0123456789" "dial numeral"

echo
echo "Fonts are OFL. src/fonts/DINish-OFL.txt travels with them; see NOTICE."
