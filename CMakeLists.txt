cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME pvz-with-hud)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

set(APP_SOURCES
)

set(PVZ_SOURCES
    libs/pvz/src/app.c
    libs/pvz/src/game.c
    libs/pvz/src/game_types.c
    libs/pvz/src/input.c
    libs/pvz/src/placeholder_scene.c
    libs/pvz/src/play_scene.c
    libs/pvz/src/presentation.c
    libs/pvz/src/pvz_config.c
    libs/pvz/src/pvz_rect.c
    libs/pvz/src/pvz_utils.c
    libs/pvz/src/render_assets.c
)

set(TFT_DISPLAY_SOURCES
    libs/tft_display/ili9488.c
)

set(HUB75_DISPLAY_SOURCES
    libs/hub75_display/hub75.c
)

set(TOF_SOURCES
    libs/tof/src/tof_sensor.c
)

add_library(pvz STATIC)
target_sources(pvz PRIVATE ${PVZ_SOURCES})
target_include_directories(pvz PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/pvz/include
)
target_link_libraries(pvz PUBLIC m)

add_library(tft_display STATIC)
target_sources(tft_display PRIVATE ${TFT_DISPLAY_SOURCES})
target_include_directories(tft_display PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/tft_display
)
target_link_libraries(tft_display PUBLIC stm32cubemx)

add_library(hub75_display STATIC)
target_sources(hub75_display PRIVATE ${HUB75_DISPLAY_SOURCES})
target_include_directories(hub75_display PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/hub75_display
)
target_link_libraries(hub75_display PUBLIC stm32cubemx)

add_library(tof STATIC)
target_sources(tof PRIVATE ${TOF_SOURCES})
target_include_directories(tof PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/tof/include
)
target_link_libraries(tof PUBLIC stm32cubemx)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${APP_SOURCES})

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    pvz
    tft_display
    hub75_display
    tof
)
