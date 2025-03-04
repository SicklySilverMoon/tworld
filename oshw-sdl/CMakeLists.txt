add_library(oshw-sdl STATIC "")
target_sources(oshw-sdl PRIVATE
    ../generic/generic.h
    ../generic/in.c
    ../generic/tile.c
    ../generic/timer.c
    oshwbind.h
    oshwbind.c
    sdlerr.c
    sdlgen.h
    sdlin.c
    sdloshw.c
    sdlout.c
    sdlsfx.h
    sdlsfx.c
    sdltext.c
)

target_include_directories(oshw-sdl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/generic
    ${SDL_INCLUDE_DIR}
)

target_link_libraries(oshw-sdl PUBLIC ${SDL_LIBRARY})
