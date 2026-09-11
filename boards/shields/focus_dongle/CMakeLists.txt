# Derived from carrefinho/prospector-zmk-module (MIT) — see NOTICE.
# Changed: sources for widgets and fonts this module does not have are dropped.

if(CONFIG_SHIELD_FOCUS_DONGLE)
  zephyr_library()
  zephyr_library_sources(${ZEPHYR_BASE}/misc/empty_file.c)
  zephyr_library_include_directories(${ZEPHYR_LVGL_MODULE_DIR})
  zephyr_library_include_directories(${ZEPHYR_BASE}/lib/gui/lvgl/)
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
  # ZMK's own headers — zmk/display.h and friends live here.
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
  zephyr_library_include_directories(include)
  zephyr_library_sources(src/brightness.c)
  zephyr_library_sources(src/display_rotate_init.c)
  zephyr_library_sources(src/status_screen.c)
endif()
