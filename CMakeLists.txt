cmake_minimum_required(VERSION 3.18)
project(visualization.fishbmc)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(glm REQUIRED)

if(NOT WIN32 AND (APP_RENDER_SYSTEM STREQUAL "gl" OR NOT APP_RENDER_SYSTEM))
  find_package(OpenGl REQUIRED)
  set(DEPLIBS ${OPENGL_LIBRARIES})
  set(includes ${OPENGL_INCLUDE_DIR})
  add_definitions(${OPENGL_DEFINITIONS})
else()
  find_package(OpenGLES REQUIRED)
  set(DEPLIBS ${OPENGLES_LIBRARIES})
  set(includes ${OPENGLES_INCLUDE_DIR})
  add_definitions(${OPENGLES_DEFINITIONS})
endif()

add_definitions(-DHAS_SDL_OPENGL)
include_directories(${includes}
                    ${GLM_INCLUDE_DIR}
                    ${KODI_INCLUDE_DIR}/..) # Hack way with "/..", need bigger Kodi cmake rework to match right include ways (becomes done in future)

set(FISH_SOURCES src/fishbmc_addon.cpp
                 src/fische/analyst.cpp
                 src/fische/audiobuffer.cpp
                 src/fische/blurengine.cpp
                 src/fische/cpudetect.c
                 src/fische/fische_c.cpp
                 src/fische/fische.cpp
                 src/fische/screenbuffer.cpp
                 src/fische/vector.cpp
                 src/fische/vectorfield.cpp
                 src/fische/wavepainter.cpp)

set(FISH_HEADERS src/fishbmc_addon.h
                 src/fische/analyst.hpp
                 src/fische/audiobuffer.hpp
                 src/fische/blurengine.hpp
                 src/fische/cpudetect.h
                 src/fische/fische.h
                 src/fische/fische.hpp
                 src/fische/screenbuffer.hpp
                 src/fische/vector.hpp
                 src/fische/vectorfield.hpp
                 src/fische/wavepainter.hpp)

build_addon(visualization.fishbmc FISH DEPLIBS)

include(CPack)
