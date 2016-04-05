IF                      ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
  MESSAGE               ( FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the source code and call cmake from there" )
ENDIF                   ( )

CMAKE_MINIMUM_REQUIRED  ( VERSION 2.6 )
PROJECT                 ( LibSteamController C )

OPTION                  ( BUILD_STATIC_LIB       "Build static library"         FALSE )

SET                     ( SOURCES
                          steamcontroller_linux.c
                          steamcontroller_win32.c

                          steamcontroller_feedback.c
                          steamcontroller_setup.c
                          steamcontroller_state.c
                          steamcontroller_wireless.c
                        )

IF                      ( BUILD_STATIC_LIB )
  ADD_LIBRARY           ( SteamController STATIC ${SOURCES} )
ELSE                    ( )
  ADD_LIBRARY           ( SteamController SHARED ${SOURCES} )

  SET_TARGET_PROPERTIES ( SteamController  PROPERTIES
                          VERSION     "0.1.0"
                          SOVERSION   "0.1" )
ENDIF                   ( )

ADD_DEFINITIONS         ( -DSTEAMCONTROLLER_BUILDING_LIBRARY ) 

IF                      ( WIN32 )
  TARGET_LINK_LIBRARIES ( SteamController setupapi hid )
ENDIF                   ( )

ADD_EXECUTABLE          ( SteamControllerExample example.c )
TARGET_LINK_LIBRARIES   ( SteamControllerExample SteamController )

INSTALL                 ( TARGETS SteamController
                          RUNTIME DESTINATION bin
                          LIBRARY DESTINATION lib
                          ARCHIVE DESTINATION lib
                        )

INSTALL                 ( FILES       steamcontroller.h
                          DESTINATION include
                        )