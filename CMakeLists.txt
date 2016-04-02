IF                      ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
  MESSAGE               ( FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the source code and call cmake from there" )
ENDIF                   ( )

CMAKE_MINIMUM_REQUIRED  ( VERSION 2.6 )
PROJECT                 ( LibSteamController C )

OPTION                  ( BUILD_STATIC_LIB       "Build static library"         FALSE )

IF                      ( BUILD_STATIC_LIB )
  ADD_LIBRARY           ( SteamController STATIC steamcontroller_linux.c )
ELSE                    ( )
  ADD_LIBRARY           ( SteamController SHARED steamcontroller_linux.c )
ENDIF                   ( )

SET_TARGET_PROPERTIES   ( SteamController  PROPERTIES
                          VERSION     "0.1.0"
                          SOVERSION   "0.1"
                        )
INSTALL                 ( TARGETS SteamController
                          RUNTIME DESTINATION bin
                          LIBRARY DESTINATION lib
                          ARCHIVE DESTINATION lib
                        )

INSTALL                 ( FILES       steamcontroller.h
                          DESTINATION include
                        )