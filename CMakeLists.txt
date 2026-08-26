# Derived from carrefinho/prospector-zmk-module (MIT) — see NOTICE.

zephyr_include_directories(include)
zephyr_library()

# ZMK's own headers — zmk/event_manager.h and friends. The shield library sets
# this too; both need it, because both compile code that talks to ZMK.
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)

# The timer core is display-agnostic and builds on EVERY target, not just the
# one with the screen. A split peripheral compiles the same keymap as the
# dongle, so it has to resolve the timer behaviours, and those call into this.
zephyr_library_sources(src/timer.c)
zephyr_library_sources(src/events/timer_state_changed.c)

if(CONFIG_SHIELD_FOCUS_DONGLE)
        # The display driver shadows Zephyr's own — see drivers/display/.
        add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/drivers/display)
endif()
