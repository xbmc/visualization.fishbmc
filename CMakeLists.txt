project(visualization.fishbmc)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(kodi REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_definitions(-DHAS_SDL_OPENGL)
include_directories(${OpenGL_INCLUDE_DIR}
                    ${GLEW_INCLUDE_DIR}
                    ${KODI_INCLUDE_DIR})

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

set(DEPLIBS ${OPENGL_LIBRARIES}
            ${GLEW_LIBRARIES}
            ${CMAKE_THREAD_LIBS_INIT})

build_addon(visualization.fishbmc FISH DEPLIBS)

include(CPack)
