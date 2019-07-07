add_library(steam_controller
  ../include/steam_controller/steam_controller.hpp
  steam_controller.cpp
)

target_link_libraries(steam_controller
  PRIVATE hidapi
)

target_include_directories(steam_controller
  PUBLIC ../include/
)

install(TARGETS ${TARGET_NAME}
  ARCHIVE DESTINATION lib
)

install(FILES
  ../include/steam_controller/steam_controller.hpp
  DESTINATION include/steam_controller
)
