set(SOURCE_FILES)

set(TARGET_NAME steam_controller)

add_library(steam_controller
  ../include/steam_controller/steam_controller.hpp
  steam_controller.cpp
  )

set(HIDAPI_PREFIX ../externals/hidapi)

if (steam_controller_PLATFORM STREQUAL PLATFORM_WIN32)
  target_sources(${TARGET_NAME}
    PRIVATE ${HIDAPI_PREFIX}/windows/hid.c)

  target_link_libraries(${TARGET_NAME}
    PUBLIC Setupapi)

  target_compile_definitions(${TARGET_NAME}
    PUBLIC NOMINMAX)

  target_compile_options(${TARGET_NAME}
    PUBLIC /wd4267)
elseif (steam_controller_PLATFORM STREQUAL PLATFORM_LINUX)
  target_sources(${TARGET_NAME}
    PRIVATE ${HIDAPI_PREFIX}/linux/hid.c)
  target_link_libraries(${TARGET_NAME}
    PRIVATE udev)
elseif (steam_controller_PLATFORM STREQUAL PLATFORM_MACOS)
  target_sources(${TARGET_NAME}
    PRIVATE ${HIDAPI_PREFIX}/mac/hid.c)

  # MAC Only
  target_link_libraries(${TARGET_NAME}
    PUBLIC "-framework CoreFoundation"
    PUBLIC "-framework IOKit")
endif ()

target_include_directories(${TARGET_NAME}
  PUBLIC ../include/
  PRIVATE ${HIDAPI_PREFIX}/include
  PRIVATE ${HIDAPI_PREFIX}/include/hidapi
  )

install(TARGETS steam_controller
  ARCHIVE DESTINATION lib
  )

install(FILES
  ../include/steam_controller/steam_controller.hpp
  DESTINATION include/steam_controller
  )
