# Derived from carrefinho/prospector-zmk-module (MIT) — see NOTICE.

zephyr_include_directories(include)
zephyr_library()

if(CONFIG_SHIELD_FOCUS_DONGLE)
        # The display driver shadows Zephyr's own — see drivers/display/.
        add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/drivers/display)
endif()
