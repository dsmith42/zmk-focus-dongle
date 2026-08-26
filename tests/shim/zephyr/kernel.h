/*
 * Host-test stand-in for Zephyr's kernel header.
 *
 * The timer core needs exactly one thing from Zephyr — a monotonic clock — so
 * that is all this provides. The test drives it directly, which is the whole
 * reason these tests can run on the host in milliseconds rather than needing a
 * target build to watch real minutes go by.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

int64_t k_uptime_get(void);
