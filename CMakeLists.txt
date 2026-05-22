if(CONFIG_ZMK_BEHAVIOR_CLK_OR_KEY
   OR CONFIG_ZMK_BEHAVIOR_VOWEL_AUTO
   OR CONFIG_ZMK_INPUT_PROCESSOR_TYPING_MODE_EXIT
   OR CONFIG_ZMK_INPUT_PROCESSOR_SCROLL_MODE)
  zephyr_library()
  zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

  zephyr_library_sources(src/typing_mode.c)

  zephyr_library_sources_ifdef(CONFIG_ZMK_BEHAVIOR_CLK_OR_KEY
    src/behaviors/behavior_clk_or_key.c)

  zephyr_library_sources_ifdef(CONFIG_ZMK_BEHAVIOR_VOWEL_AUTO
    src/behaviors/behavior_vowel_auto.c)

  if(CONFIG_ZMK_POINTING)
    zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_PROCESSOR_TYPING_MODE_EXIT
      src/input_processors/input_processor_typing_mode_exit.c)
    zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_PROCESSOR_SCROLL_MODE
      src/input_processors/input_processor_scroll_mode.c)
  endif()
endif()
