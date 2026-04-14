cmake_minimum_required(VERSION 3.20.0)

set(APP "t114-support")
set(CXX_STANDARD C++17)
# list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(${APP})

file(GLOB_RECURSE APP_C_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/boards/*.c
)

file(GLOB_RECURSE APP_H_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/boards/*.h
)

target_sources(app PRIVATE ${APP_C_SOURCES})
target_include_directories(app PRIVATE src ${ZEPHYR_BASE}/boards/heltec/heltec_t114_v2)

set(ALLOWED_BOARDS "promicro_nrf52840;heltec_t114_v2")
list(FIND ALLOWED_BOARDS ${BOARD} BOARD_INDEX)
if(BOARD_INDEX EQUAL -1)
  message(FATAL_ERROR 
    "This application is intended to be built only for the following boards: ${ALLOWED_BOARDS}. Current BOARD is: ${BOARD}")
endif()


add_custom_target(fix
    COMMAND clang-format -i --style=file:${ZEPHYR_BASE}/.clang-format ${APP_C_SOURCES} ${APP_H_SOURCES}
    COMMENT "Formatting source files with Zephyr style"
)