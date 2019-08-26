cmake_minimum_required(VERSION 3.5)
project(visualization.fishbmc)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(Threads REQUIRED)

if(NOT WIN32)
  if(APP_RENDER_SYSTEM STREQUAL "gl" OR NOT APP_RENDER_SYSTEM)
    find_package(OpenGl REQUIRED)
    set(DEPLIBS ${OPENGL_LIBRARIES})
    set(includes ${OPENGL_INCLUDE_DIR})
    add_definitions(${OPENGL_DEFINITIONS})
  elseif(APP_RENDER_SYSTEM STREQUAL "gles")
    find_package(OpenGLES REQUIRED)
    set(DEPLIBS ${OPENGLES_LIBRARIES})
    set(includes ${OPENGLES_INCLUDE_DIR})
    add_definitions(${OPENGLES_DEFINITIONS})
  endif()
endif()

add_definitions(-DHAS_SDL_OPENGL)
include_directories(${includes}
                    ${KODI_INCLUDE_DIR}/.. # Hack way with "/..", need bigger Kodi cmake rework to match right include ways (becomes done in future)
                    ${PROJECT_SOURCE_DIR}/lib)

set(FISH_SOURCES src/analyst.c
                 src/audiobuffer.c
                 src/blurengine.c
                 src/cpudetect.c
                 src/fische.c
                 src/fishbmc_addon.cpp
                 src/screenbuffer.c
                 src/vector.c
                 src/vectorfield.c
                 src/wavepainter.c)

set(FISH_HEADERS src/analyst.h
                 src/audiobuffer.h
                 src/blurengine.h
                 src/fische.h
                 src/fishbmc_addon.h
                 src/fische_internal.h
                 src/screenbuffer.h
                 src/vectorfield.h
                 src/vector.h
                 src/wavepainter.h)

list(APPEND DEPLIBS ${CMAKE_THREAD_LIBS_INIT})

build_addon(visualization.fishbmc FISH DEPLIBS)

include(CPack)
