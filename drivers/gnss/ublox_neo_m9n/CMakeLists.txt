zephyr_library()
zephyr_library_sources(src/ublox_neo_m9n.c)
zephyr_library_include_directories(include)

target_include_directories(app PUBLIC include)
