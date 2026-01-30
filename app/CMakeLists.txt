cmake_minimum_required(VERSION 3.20.0)

set(APP "t114-support")
set(CXX_STANDARD C++17)
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(${APP})

target_sources(app PRIVATE src/main.cpp)
target_include_directories(app PRIVATE src)

set(ALLOWED_BOARDS "promicro_nrf52840;heltec_t114")
list(FIND ALLOWED_BOARDS ${BOARD} BOARD_INDEX)
if(BOARD_INDEX EQUAL -1)
  message(FATAL_ERROR 
    "This application is intended to be built only for the following boards: ${ALLOWED_BOARDS}. Current BOARD is: ${BOARD}")
endif()