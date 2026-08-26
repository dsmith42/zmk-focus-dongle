/*
 * Host-test stand-in for ZMK's event manager.
 *
 * Reduces the event macros to a plain function declaration. These tests cover
 * the timer's own logic, not ZMK's event plumbing — so the test provides the
 * raise function and records the calls.
 */

#pragma once

#define ZMK_EVENT_DECLARE(type) int raise_##type(struct type ev)
#define ZMK_EVENT_IMPL(type)
