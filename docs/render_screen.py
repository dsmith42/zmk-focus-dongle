#!/usr/bin/env python3
"""Render the status screen as PNGs, for the README and for pull requests.

⚠️ THIS IS NOT A SECOND IMPLEMENTATION, and it must never become one.

The tree this was derived from had a Python renderer that redrew the dial from
constants it scraped — a parallel implementation that drifted from the firmware
and could not have caught it. Everything geometric here comes from what the C
actually produced:

  - the dial's angles, numerals and colour ROLES are read from
    tests/dial.snapshot, which is the output of tests/snapshot_dial.c
  - the palette, the derivations (lighten/scale) and the placement constants are
    parsed out of src/theme.c, include/focus/theme.h, src/status_widget.c and
    include/focus/dial_widget.h
  - the typefaces and sizes are parsed out of scripts/gen_fonts.sh

So the only things invented here are the SCENE — which layer, which profile,
which modifiers are held — and the drawing calls. If a constant moves in the C,
this follows; if it cannot parse one, it fails rather than guessing.

Usage:  python3 docs/render_screen.py [outdir]
Needs:  Pillow, and network on first run (it fetches the same TTFs gen_fonts.sh
        bakes from, rather than vendoring a megabyte of font into the repo).
"""

import os
import re
import subprocess
import sys
import tempfile
import urllib.request
from math import cos, radians, sin

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PANEL = (280, 240)


def read(path):
    with open(os.path.join(ROOT, path)) as f:
        return f.read()


def defines(path):
    """Every `#define NAME <int>` in a file, as a dict."""
    out = {}
    for name, value in re.findall(r"#define\s+(\w+)\s+(0[xX][0-9a-fA-F]+|-?\d+)", read(path)):
        out[name] = int(value, 0)
    return out


def need(d, name, where):
    if name not in d:
        sys.exit(f"{name} is gone from {where} — update docs/render_screen.py")
    return d[name]


# --- the palette, from the firmware's own mapper --------------------------

THEME = {**defines("src/theme.c"), **defines("include/focus/theme.h")}
DIAL = defines("include/focus/dial_widget.h")
STATUS = defines("src/status_widget.c")


def lighten(c, pct):
    r, g, b = (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF
    return tuple(int(v + (255 - v) * pct / 100) for v in (r, g, b))


def scale(c, pct):
    return tuple(int(((c >> s) & 0xFF) * pct / 100) for s in (16, 8, 0))


def rgb(c):
    return ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF)


WEDGE = need(THEME, "THEME_WEDGE", "src/theme.c")
ARMED_LEVEL = need(THEME, "THEME_ARMED_LEVEL", "src/theme.c")
HAND_LIFT = need(THEME, "THEME_HAND_LIFT", "src/theme.c")

ROLE = {
    "theme": rgb(WEDGE),
    "dim": scale(WEDGE, ARMED_LEVEL),
    "grey": rgb(need(THEME, "THEME_GREY", "src/theme.c")),
    "accent": rgb(need(THEME, "THEME_ACCENT", "src/theme.c")),
    "idle": rgb(need(THEME, "THEME_IDLE", "src/theme.c")),
    "ok": rgb(need(THEME, "THEME_OK", "src/theme.c")),
    "low": rgb(need(THEME, "THEME_LOW", "src/theme.c")),
}
FACE = rgb(need(THEME, "FOCUS_COLOR_FACE", "focus/theme.h"))
BLOCK = rgb(need(THEME, "FOCUS_COLOR_BLOCK", "focus/theme.h"))
TICK = rgb(need(THEME, "FOCUS_COLOR_TICK", "focus/theme.h"))
HUB = rgb(need(THEME, "FOCUS_COLOR_HUB", "focus/theme.h"))


def hand_colour(wedge_role):
    return lighten(WEDGE, HAND_LIFT) if wedge_role == "theme" else rgb(WEDGE)


# --- fonts, from the generator --------------------------------------------

GEN = read("scripts/gen_fonts.sh")
URLS = dict(re.findall(r'^(\w+)="(https://[^"]+)"', GEN, re.M))
BAKED = {
    name: (URLS[var.strip("$")], int(size))
    for name, var, size in re.findall(
        r'generate\s+"(\w+)"\s+"(\$\w+)"\s+(\d+)', GEN
    )
}

_fontdir = os.path.join(tempfile.gettempdir(), "zmk-focus-dongle-fonts")


def font(baked_name):
    url, size = BAKED[baked_name]
    os.makedirs(_fontdir, exist_ok=True)
    path = os.path.join(_fontdir, url.rsplit("/", 1)[-1])
    if not os.path.exists(path):
        print(f"fetching {os.path.basename(path)}...")
        urllib.request.urlretrieve(url, path)
    return ImageFont.truetype(path, size)


# --- the states, from the C's own output ----------------------------------


def snapshot():
    """Parse tests/dial.snapshot into {label: {...}}."""
    states, current = {}, None
    for line in read("tests/dial.snapshot").splitlines():
        if not line.strip():
            continue
        if not line.startswith(" "):
            current = states.setdefault(line.strip(), {})
            continue
        key, rest = line.split(None, 1)
        parts = rest.split()
        if key == "ring":
            current["ring"] = None if parts[0] == "hidden" else (int(parts[0]), int(parts[2]))
        elif key in ("numeral", "wedge_deg"):
            current[key] = (int(parts[0]), parts[1])
        else:
            current[key] = int(parts[0])
    return states


# --- drawing ---------------------------------------------------------------

CX, CY, R = DIAL["DIAL_CX"], DIAL["DIAL_CY"], DIAL["DIAL_R"]
RING_R = R + 13  # DIAL_RING_R is (DIAL_R + 13); the offset is what is written
RING_W = DIAL["DIAL_RING_W"]
HAND_R = RING_R - RING_W // 2
HUB_R = DIAL["DIAL_HUB_R"]
TICKS = DIAL["DIAL_TICK_COUNT"]
TICK_IN, TICK_OUT = R + 21, R + 29

INSET = STATUS["STATUS_INSET"]
CORNER = STATUS["STATUS_CORNER_INSET"]
PROFILE_Y = STATUS["STATUS_PROFILE_Y"]
BATTERY_Y = STATUS["STATUS_BATTERY_Y"]
BATTERY_PITCH = STATUS["STATUS_BATTERY_PITCH"]
MODS_PITCH = STATUS["STATUS_MODS_PITCH"]
MODS_Y = STATUS["STATUS_MODS_Y"]
CARET_DROP = STATUS["STATUS_MODS_CARET_DROP"]

MOD_GLYPHS = ["\u2318", "\u2325", "\u2303", "\u21E7"]  # GACS, as the widget lays them out


def arc_box(r):
    return [CX - r, CY - r, CX + r, CY + r]


def draw_dial(d, st):
    for i in range(TICKS):
        a = radians(360 * i / TICKS - 90)
        mid = TICK_IN + (TICK_OUT - TICK_IN) / 2
        x, y = CX + mid * cos(a), CY + mid * sin(a)
        d.ellipse([x - 1.5, y - 1.5, x + 1.5, y + 1.5], fill=TICK)

    d.ellipse(arc_box(R), fill=FACE)
    d.pieslice(arc_box(R), -90, -90 + st["track_deg"], fill=BLOCK)

    wedge_deg, wedge_role = st["wedge_deg"]
    if wedge_deg:
        d.pieslice(arc_box(R), -90, -90 + wedge_deg, fill=ROLE[wedge_role])

    if st["ring"]:
        ring_wedge, ring_track = st["ring"]
        d.arc(arc_box(RING_R), -90, -90 + ring_track, fill=BLOCK, width=RING_W)
        if ring_wedge:
            d.arc(arc_box(RING_R), -90, -90 + ring_wedge, fill=ROLE[wedge_role], width=RING_W)

    a = radians(st["hand_deg"] - 90)
    d.line([CX, CY, CX + HAND_R * cos(a), CY + HAND_R * sin(a)],
           fill=hand_colour(wedge_role), width=4)
    d.ellipse([CX - HUB_R, CY - HUB_R, CX + HUB_R, CY + HUB_R], fill=HUB)

    numeral, numeral_role = st["numeral"]
    d.text((PANEL[0] - INSET, 22), str(numeral), font=font("DINish_Medium_32"),
           fill=ROLE[numeral_role], anchor="rt")


def draw_status(d, layer, profile, batteries, held):
    circled = len(profile) == 1
    d.text((PANEL[0] - INSET, PROFILE_Y), profile,
           font=font("NotoSymbols_Regular_28" if circled else "DINish_Medium_20"),
           fill=ROLE["grey"], anchor="rt")

    for i, level in enumerate(batteries):
        x = PANEL[0] - INSET - (len(batteries) - 1 - i) * BATTERY_PITCH
        d.text((x, BATTERY_Y), str(level), font=font("DINish_Medium_24"),
               fill=ROLE["low" if level < 15 else "ok"], anchor="rt")

    d.text((CORNER, PANEL[1] + MODS_Y - 3), layer.upper(),
           font=font("DINish_Medium_20"), fill=ROLE["accent"], anchor="ls")

    for i, glyph in enumerate(MOD_GLYPHS):
        x = PANEL[0] - CORNER - (len(MOD_GLYPHS) - 1 - i) * MODS_PITCH
        # LVGL's y grows downward, so the widget's +CARET_DROP is a drop here too.
        y = PANEL[1] + MODS_Y + (CARET_DROP if glyph == "\u2303" else 0)
        d.text((x, y), glyph, font=font("JuliaMono_Regular_28"),
               fill=ROLE["accent" if held[i] else "idle"], anchor="rs")


def render(state, layer, profile, batteries, held):
    img = Image.new("RGB", PANEL, (0, 0, 0))
    d = ImageDraw.Draw(img)
    draw_dial(d, state)
    draw_status(d, layer, profile, batteries, held)
    return img


SCENES = [
    # snapshot label,            file stem,  layer,   profile, batteries, held (GACS)
    ("armed 45, not started",    "armed",    "Focus", "\u2460", [84, 61], [0, 0, 0, 0]),
    ("running 21 of 45",         "running",  "Base",  "\u2460", [84, 61], [1, 1, 0, 0]),
    ("overrun 15 past a 45",     "overrun",  "Base",  "USB",    [84, 9],  [0, 0, 0, 0]),
]


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "docs/images")
    os.makedirs(outdir, exist_ok=True)

    subprocess.run(["./tests/run.sh"], cwd=ROOT, check=True,
                   stdout=subprocess.DEVNULL)  # the snapshot must be current
    states = snapshot()

    for label, stem, layer, profile, batteries, held in SCENES:
        if label not in states:
            sys.exit(f"'{label}' is no longer in tests/dial.snapshot — update SCENES")
        img = render(states[label], layer, profile, batteries, held)
        path = os.path.join(outdir, f"screen-{stem}.png")
        img.save(path)
        img.resize((PANEL[0] * 2, PANEL[1] * 2), Image.LANCZOS).save(
            path.replace(".png", "@2x.png"))
        print(f"wrote {os.path.relpath(path, ROOT)}")


if __name__ == "__main__":
    main()
