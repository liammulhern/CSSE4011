# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set(ZEPHYR_EXTRA_MODULES
    ${CMAKE_SOURCE_DIR}/../../nrf-sdk/subsys
    ${CMAKE_SOURCE_DIR}/../../nrf-sdk/drivers
)

project(tracker)

include_directories(include)

add_subdirectory(src)
