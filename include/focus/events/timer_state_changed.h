#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct focus_timer_state_changed {
    bool running;
};

ZMK_EVENT_DECLARE(focus_timer_state_changed);
