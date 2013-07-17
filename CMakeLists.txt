PROJECT(visualization.fishbmc)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

include(AddonVersion.cmake)
addon_version(visualization.fishbmc FISH)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

OPTION(PACKAGE_ZIP "Package Zip file?" OFF)

FIND_PACKAGE(Threads REQUIRED)

FIND_PATH(XBMC_INCLUDE_DIR xbmc/xbmc_addon_dll.h)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLEW REQUIRED)
ADD_DEFINITIONS(-DHAS_SDL_OPENGL)
INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIR}
                    ${GLEW_INCLUDE_DIR}
                    ${XBMC_INCLUDE_DIR})

SET(SOURCES src/analyst.c
            src/audiobuffer.c
            src/blurengine.c
            src/cpudetect.c
            src/fische.c
            src/fishbmc_addon.cpp
            src/screenbuffer.c
            src/vector.c
            src/vectorfield.c
            src/wavepainter.c)

ADD_LIBRARY(fishbmc ${SOURCES})
IF(NOT WIN32)
  TARGET_LINK_LIBRARIES(fishbmc ${OPENGL_LIBRARIES}
                                ${GLEW_LIBRARIES}
                                ${CMAKE_THREAD_LIBS_INIT}
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-s")
  set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "-s")
ENDIF(NOT WIN32)
SET_TARGET_PROPERTIES(fishbmc PROPERTIES VERSION ${FISH_VERSION}
                                         SOVERSION 13.0
                                         PREFIX "")

IF(PACKAGE_ZIP)
  # Pack files together to create an archive
  INSTALL(DIRECTORY ${PROJECT_NAME} DESTINATION ./)
  INSTALL(TARGETS fishbmc DESTINATION visualization.fishbmc)
  SET(CPACK_GENERATOR "ZIP")
  SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
  SET(CPACK_PACKAGE_FILE_NAME ${CMAKE_PROJECT_NAME}-${FISH_VERSION})
  INCLUDE(CPack)
ELSE(PACKAGE_ZIP)
  INSTALL(TARGETS fishbmc DESTINATION lib/xbmc/addons/visualization.fishbmc)
  INSTALL(DIRECTORY visualization.fishbmc DESTINATION share/xbmc/addons)
ENDIF(PACKAGE_ZIP)
