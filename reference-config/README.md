# Reference config

A ZMK module is not a buildable thing on its own — it only compiles when a
keyboard configuration pulls it in. This directory is the smallest such
configuration, and exists so CI can prove the module builds.

It is a keyless split central: a mock kscan, a fictional 2x2 matrix, and nothing
else. That is the shape of a dongle. ZMK requires a transform and a keymap of
matching size; four positions satisfy that and leave room to bind this module's
behaviours as they arrive. The module draws a screen and owns a timer — nothing
about it depends on what is on the other end of the link.

## Why it lives at the repository root

ZMK's reusable workflow does a non-recursive `mkdir` on `config_path`, so a
nested location such as `tests/reference-config` fails before the build starts.
Single-level or nothing.

## Adding a build target

Append to `build.yaml`. The CI matrix is generated from that file and the
workflow needs no changes.
