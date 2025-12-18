cmake_minimum_required(VERSION 3.20.0)

set(APP "app")
set(CXX_STANDARD C++17)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(${APP})

target_sources(app PRIVATE src/main.cpp)

set(ALLOWED_BOARDS "nrf5340dk_nrf5340_cpuapp;native_posix_64;promicro_nrf52840")
list(FIND ALLOWED_BOARDS ${BOARD} BOARD_INDEX)
if(BOARD_INDEX EQUAL -1)
  message(FATAL_ERROR 
    "This application is intended to be built only for the following boards: ${ALLOWED_BOARDS}. Current BOARD is: ${BOARD}")
endif()