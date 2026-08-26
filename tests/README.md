# Tests

Host tests for logic that does not need a target build.

The timer core is arithmetic over a monotonic clock, so it can be tested by
controlling that clock directly — a block that would take 45 real minutes to
overrun takes none at all here. That is the whole reason these run on the host:
seconds in CI rather than minutes, and no Zephyr workspace to stand up.

`shim/` provides the two things the timer needs from its environment: a clock
(`zephyr/kernel.h`) and an event raise (`zmk/event_manager.h`). These tests cover
the timer's own logic, not ZMK's plumbing.

## Running them

```sh
cc -std=c99 -Wall -Wextra -Werror -I include -I tests/shim \
   -o /tmp/test_timer tests/test_timer.c src/timer.c && /tmp/test_timer
```

## What is worth testing here

Anything that is pure logic and has edge cases: deadline arithmetic, the rules
about when a block is locked, what survives a block. Anything that needs LVGL, a
display or real ZMK events does not belong — that is what the firmware build and
a flash are for.
