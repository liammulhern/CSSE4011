
zephyr_library()
zephyr_library_sources_ifdef(CONFIG_UBLOX_NEO_M9N src/ublox-neo-m9n.c)
zephyr_library_include_directories(include)
