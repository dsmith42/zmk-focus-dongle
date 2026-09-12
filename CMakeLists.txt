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

# The behaviours build on every target too, and for the same reason: the keymap
# that binds them is compiled by the peripherals as well as the dongle. Each is
# gated only on its own devicetree node existing, so a keymap that binds none of
# them costs nothing.
if(CONFIG_DT_HAS_ZMK_BEHAVIOR_FOCUS_TIME_ENABLED)
        zephyr_library_sources(src/behaviors/behavior_focus_time.c)
endif()

if(CONFIG_DT_HAS_ZMK_BEHAVIOR_FOCUS_START_ENABLED)
        zephyr_library_sources(src/behaviors/behavior_focus_start.c)
endif()

if(CONFIG_DT_HAS_ZMK_BEHAVIOR_FOCUS_STOP_ENABLED)
        zephyr_library_sources(src/behaviors/behavior_focus_stop.c)
endif()

if(CONFIG_SHIELD_FOCUS_DONGLE)
        # The display driver shadows Zephyr's own — see drivers/display/.
        add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/drivers/display)

        # The view models and their widgets. Display-only — nothing a keymap can
        # reach — so unlike the timer core they are correctly behind the shield
        # guard.
        #
        # They are compiled HERE rather than in the shield's own CMakeLists
        # because ZEPHYR_CURRENT_MODULE_DIR is only set while Zephyr's module
        # system is processing this file. The shield CMakeLists is pulled in by
        # the shield mechanism instead, where the variable is empty — a source
        # path there resolved to a bare "/src/dial.c" and failed the dongle
        # build.
        zephyr_library_sources(src/dial.c)
        zephyr_library_sources(src/dial_widget.c)
        zephyr_library_sources(src/status.c)
        zephyr_library_sources(src/status_widget.c)

        # Role -> colour, shared by both widgets. The one place a theme changes.
        zephyr_library_sources(src/theme.c)

        # Baked glyph subsets. Display-only, so they sit behind the shield guard
        # with the widgets that draw them. Regenerate with scripts/gen_fonts.sh.
        zephyr_library_sources(src/fonts/DINish_Medium_32.c)
        zephyr_library_sources(src/fonts/DINish_Medium_24.c)
        zephyr_library_sources(src/fonts/DINish_Medium_20.c)
        zephyr_library_sources(src/fonts/NotoSymbols_Regular_28.c)
endif()
