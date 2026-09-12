#!/usr/bin/env bash
#
# Regenerate the glyph subsets used by the display.
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
# The one non-Latin face is JuliaMono, which carries all four modifier glyphs.
# Also OFL, and also baked down to the glyphs used.
#
# Requires node (for npx) and network. The TTFs are downloaded rather than
# vendored, both to keep the repo small and to make the OFL provenance obvious.

set -euo pipefail

BPP=4

# DINish: SIL Open Font License 1.1. https://github.com/playbeing/dinish
DINISH="https://raw.githubusercontent.com/playbeing/dinish/master/fonts/ttf/DINish/DINish-Medium.ttf"

# JuliaMono: SIL Open Font License 1.1. https://github.com/cormullion/juliamono
JULIAMONO="https://raw.githubusercontent.com/cormullion/juliamono/master/JuliaMono-Regular.ttf"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out_dir="$repo_root/src/fonts"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

# generate <out-name> <ttf-url> <size> <set> <what-it-is>
#
# <set> is either a literal list of characters or an lv_font_conv range such as
# 0x20-0x7E. Ranges exist for the layer label, whose text this module does not
# author -- the name comes from someone else's keymap, so the set has to be the
# whole printable block rather than the characters we happen to draw.
generate() {
    local out_name="$1" url="$2" size="$3" set="$4" what="$5"
    local ttf="${url##*/}"
    local selector=(--symbols "$set")

    case "$set" in
    0x*) selector=(--range "$set") ;;
    esac

    if [ ! -f "$work/$ttf" ]; then
        echo "Fetching $ttf..."
        curl -sSL --fail -o "$work/$ttf" "$url"
    fi

    echo "Converting ${what} at ${size}px, ${BPP}bpp..."
    npx -y lv_font_conv@1.5.2 \
        --font "$work/$ttf" \
        --bpp "$BPP" \
        --size "$size" \
        --no-compress \
        --format lvgl \
        "${selector[@]}" \
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
generate "DINish_Medium_32" "$DINISH" 32 "0123456789" "dial numeral"

# The layer label, and the two text forms of the profile indicator ("USB", and
# the "B n" fallback past the circled digits). Printable ASCII, because the layer
# name comes from the consumer's keymap: baking only the characters one keymap
# uses would give everyone else tofu, on a device with no log to explain it.
# Costs ~4 KB against ~380 KB of free flash.
generate "DINish_Medium_20" "$DINISH" 20 "0x20-0x7E" "layer label"

# Battery percentages and the BLE profile. Digits, plus the three letters of
# "USB" -- the profile is a plain number and needs a word only when wired.
#
# The batteries sit side by side with position carrying which half is which, so
# there is no "L"/"R" and no percent sign.
generate "DINish_Medium_24" "$DINISH" 24 "0123456789USB" "battery and profile numerals"

# The held modifiers, in GACS order. Four glyphs from ONE face, which was the
# open question of this rung: the spec's plan was to merge Noto Sans Symbols 1
# and 2, because ⌃ lives in the first and ⌘⌥⇧ in the second. Measured coverage
# says JuliaMono carries all four, and it is OFL.
#
# That matters for more than tidiness. lv_font_conv's --size is global per
# conversion, so a two-family merge cannot correct for the fact that Noto's ⌃ is
# 9.0px wide against its ⌥ at 17.3px -- nearly half, because they are different
# faces. JuliaMono is monospaced, so its four sit in 10.4..11.6px by
# construction. One face, no fallback chain, no size-per-source trick.
#
# 28px puts ⌘ at 14.6px of ink against the layer label's 14.2px cap height, so
# the two ends of the bottom row read as one line rather than two sizes.
generate "JuliaMono_Regular_28" "$JULIAMONO" 28 "⌘⌥⌃⇧" "modifier glyphs"

echo
echo "Fonts are OFL. Their licence texts travel with them in src/fonts/; see NOTICE."
