# Reference config

A ZMK module is not a buildable thing on its own — it only compiles when a
keyboard configuration pulls it in. This directory is the smallest such
configuration, and exists so CI can prove the module builds.

It is a keyless split central: a mock kscan, the shared Corne five-column
layout so the transform does not have to be invented here, and nothing else.
That is the shape of a dongle.

## Why it lives at the repository root

ZMK's reusable workflow does a non-recursive `mkdir` on `config_path`, so a
nested location such as `tests/reference-config` fails before the build starts.
Single-level or nothing.

## Adding a build target

Append to `build.yaml`. The CI matrix is generated from that file and the
workflow needs no changes.
