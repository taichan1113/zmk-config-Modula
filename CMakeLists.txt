cmake_minimum_required(VERSION 3.20.0)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_MODULA_BEHAVIOR_POINTER_HOLD_TAP
  src/behavior_pointer_hold_tap.c)
