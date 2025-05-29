zephyr_library()
zephyr_library_sources(src/lis2dh12.c)
zephyr_library_include_directories(include)

target_include_directories(app PUBLIC include)
