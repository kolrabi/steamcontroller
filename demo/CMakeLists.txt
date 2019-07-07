set(TARGET_NAME demo)

add_executable(${TARGET_NAME}
  demo.cpp)

target_link_libraries(${TARGET_NAME}
  PUBLIC steam_controller)
