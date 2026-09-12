#!/usr/bin/env sh
#
# Host tests. The same command CI runs, so a local failure is the CI failure.
#
#   ./tests/run.sh              run everything
#   ./tests/run.sh --update     accept the current dial geometry as the snapshot
#
# Nothing here needs a Zephyr workspace or a target build: it is all logic that
# was deliberately kept free of LVGL so it could be reached from the host.

set -e

cd "$(dirname "$0")/.."
CC="${CC:-cc}"
FLAGS="-std=c99 -Wall -Wextra -Werror -I include -I tests/shim"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

$CC $FLAGS -o "$OUT/timer" tests/test_timer.c src/timer.c
"$OUT/timer"

$CC $FLAGS -o "$OUT/dial" tests/test_dial.c src/dial.c
"$OUT/dial"

$CC $FLAGS -o "$OUT/status" tests/test_status.c src/status.c
"$OUT/status"

# ...and again with the target's char signedness. arm-none-eabi defaults to
# UNSIGNED char; x86 and Apple silicon hosts default to signed. The layer
# label's UTF-8 rule is exactly the kind of thing that difference hides: a
# naive `c >= 'a'` uppercase passes on the host and mangles every multi-byte
# name on the dongle, with no log and nothing but a wrong glyph to show for it.
echo "  ...and again with unsigned char:"
$CC $FLAGS -funsigned-char -o "$OUT/status-unsigned" tests/test_status.c src/status.c
"$OUT/status-unsigned"

$CC $FLAGS -o "$OUT/snapshot" tests/snapshot_dial.c src/dial.c

if [ "$1" = "--update" ]; then
    "$OUT/snapshot" > tests/dial.snapshot
    echo "dial geometry: snapshot updated — review the diff before committing"
    exit 0
fi

"$OUT/snapshot" > "$OUT/actual.snapshot"
if diff -u tests/dial.snapshot "$OUT/actual.snapshot"; then
    echo "dial geometry: snapshot matches"
else
    echo
    echo "The dial geometry changed. If that was deliberate, run:"
    echo "    ./tests/run.sh --update"
    echo "and commit the diff so the change is on the record."
    exit 1
fi
