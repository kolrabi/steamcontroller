set(TARGET_NAME hidapi)

if(steam_controller_PLATFORM STREQUAL PLATFORM_WIN32)
  add_library(${TARGET_NAME}
    windows/hid.c)

  target_link_libraries(${TARGET_NAME}
    PUBLIC Setupapi)
endif()

if(steam_controller_PLATFORM STREQUAL PLATFORM_LINUX)
  add_library(${TARGET_NAME}
    linux/hid.c)
  target_link_libraries(${TARGET_NAME}
    PRIVATE udev)
endif()

if(steam_controller_PLATFORM STREQUAL PLATFORM_MACOS)
  add_library(${TARGET_NAME}
    mac/hid.c)

  # MAC Only
  target_link_libraries(${TARGET_NAME}
    PUBLIC "-framework CoreFoundation"
    PUBLIC "-framework IOKit")
endif()


target_include_directories(${TARGET_NAME}
  INTERFACE include
  PRIVATE include/hidapi)
