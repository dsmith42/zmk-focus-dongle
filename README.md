# zmk-focus-dongle

A ZMK module for the [Prospector](https://github.com/carrefinho/prospector) desktop
dongle: a focus-block timer drawn as an analogue dial, with layer, modifiers, output
profile and peripheral battery around it.

> **Status: early.** Being rebuilt from the ground up. Not ready to use yet.

## Credit

This exists because **[carrefinho](https://github.com/carrefinho)** designed the
Prospector dongle and popularised it. If you want one,
[buy the kit from beekeeb](https://shop.beekeeb.com/products/zmk-wireless-dongle-prospector-diy-kit)
— that is where this build / addition started.

- **Hardware** — [carrefinho/prospector](https://github.com/carrefinho/prospector),
  CERN-OHL-P-2.0
- **Shield definition** — *derived from*
  [carrefinho/prospector-zmk-module](https://github.com/carrefinho/prospector-zmk-module),
  MIT. The overlays describing the display, backlight and board wiring came from there;
  see [`NOTICE`](NOTICE) and the commit history.
- **Display driver** — Zephyr's `sitronix,st7789v`, Apache-2.0, vendored with
  carrefinho's modification adding display orientation support. Upstream Zephyr
  cannot rotate this panel. See [`NOTICE`](NOTICE).

Everything above the hardware layer — the dial, the block timer, the behaviours, the
theming and the font pipeline — is new work.

## Why a separate module

carrefinho's module is a general status screen with several layouts. This one does a
single thing: it is a timer you glance at. The screen is deliberately quiet — no
flashing, no colour transitions, no state that demands attention — and it uses only
openly licensed typefaces so it can be shared.

## Licence

MIT. See [`LICENSE`](LICENSE), and [`NOTICE`](NOTICE) for the licences of the work this
builds on.
