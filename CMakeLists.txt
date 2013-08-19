PROJECT(visualization.fishbmc)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

FIND_PACKAGE(xbmc REQUIRED)
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLEW REQUIRED)

ADD_DEFINITIONS(-DHAS_SDL_OPENGL)
INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIR}
                    ${GLEW_INCLUDE_DIR}
                    ${XBMC_INCLUDE_DIR})

SET(FISH_SOURCES src/analyst.c
                 src/audiobuffer.c
                 src/blurengine.c
                 src/cpudetect.c
                 src/fische.c
                 src/fishbmc_addon.cpp
                 src/screenbuffer.c
                 src/vector.c
                 src/vectorfield.c
                 src/wavepainter.c)

SET(DEPLIBS ${OPENGL_LIBRARIES}
            ${GLEW_LIBRARIES}
            ${CMAKE_THREAD_LIBS_INIT})

build_addon(visualization.fishbmc FISH DEPLIBS)

include(CPack)
