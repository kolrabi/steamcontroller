cmake_minimum_required(VERSION 3.0)
project(steam_controller)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS KEEP_RPATHS)

# Canonize platform selection
set(steam_controller_PLATFORM Undefined)
if(WIN32)
    set(steam_controller_PLATFORM PLATFORM_WIN32)
elseif(APPLE)
    set(steam_controller_PLATFORM PLATFORM_MACOS)
elseif(UNIX AND NOT APPLE)
    set(steam_controller_PLATFORM PLATFORM_LINUX)
endif()

add_subdirectory(externals/hidapi)
add_subdirectory(source)
add_subdirectory(demo)
