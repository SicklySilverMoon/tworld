# CMake build system for Tile World 2
#
# Copyright (C) 2020 by Michael Hansen, under the GNU General Public
# License. No warranty. See COPYING for details.

cmake_minimum_required(VERSION 3.20)
project(tworld)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(OSHW "qt" CACHE STRING "OS/HW Flavor to build (sdl or qt)")
set_property(CACHE OSHW PROPERTY STRINGS qt sdl)

if(OSHW STREQUAL "qt")
    set(TWORLD_EXE "tworld2")
elseif(OSHW STREQUAL "sdl")
    set(TWORLD_EXE "tworld")
else()
    message(FATAL_ERROR "${OSHW} is not a valid OSHW setting")
endif()

if(OSHW STREQUAL "sdl")
    find_package(SDL REQUIRED)
endif()

if(OSHW STREQUAL "qt")
    set(CMAKE_AUTOUIC TRUE)
    set(CMAKE_AUTOMOC TRUE)
    set(CMAKE_AUTORCC TRUE)
    # We use SDL2 for the Qt build for audio.
    find_package(SDL2 REQUIRED)
    find_package(Qt6 COMPONENTS Core Gui Widgets Xml)
    if(NOT Qt6_FOUND)
        find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Xml)
        if(Qt5_FOUND AND NOT TARGET Qt::Core)
            # The version-generic targets were only added in Qt 5.15
            add_library(Qt::Core INTERFACE IMPORTED)
            set_target_properties(Qt::Core PROPERTIES INTERFACE_LINK_LIBRARIES "Qt5::Core")
            add_library(Qt::Gui INTERFACE IMPORTED)
            set_target_properties(Qt::Gui PROPERTIES INTERFACE_LINK_LIBRARIES "Qt5::Gui")
            add_library(Qt::Widgets INTERFACE IMPORTED)
            set_target_properties(Qt::Widgets PROPERTIES INTERFACE_LINK_LIBRARIES "Qt5::Widgets")
            add_library(Qt::Xml INTERFACE IMPORTED)
            set_target_properties(Qt::Xml PROPERTIES INTERFACE_LINK_LIBRARIES "Qt5::Xml")
        endif()
    endif()
    add_definitions(-DTWPLUSPLUS)
endif()

file(GLOB TW_SETS "${CMAKE_CURRENT_SOURCE_DIR}/sets/*.dac")
file(GLOB TW_DATA "${CMAKE_CURRENT_SOURCE_DIR}/data/*.dat"
                  "${CMAKE_CURRENT_SOURCE_DIR}/data/*.ccx")
file(GLOB TW_RES  "${CMAKE_CURRENT_SOURCE_DIR}/res/rc"
                  "${CMAKE_CURRENT_SOURCE_DIR}/res/*.bmp"
                  "${CMAKE_CURRENT_SOURCE_DIR}/res/*.txt"
                  "${CMAKE_CURRENT_SOURCE_DIR}/res/*.wav")
file(GLOB TW_MAN6 "${CMAKE_CURRENT_SOURCE_DIR}/docs/*.6")
file(GLOB TW_CCLP_DOCS "${CMAKE_CURRENT_SOURCE_DIR}/docs/CCLP*")

if(WIN32)
    file(GLOB TW_DOCS "${CMAKE_CURRENT_SOURCE_DIR}/docs/*.html")

    install(FILES ${TW_SETS} DESTINATION "${CMAKE_INSTALL_PREFIX}/sets")
    install(FILES ${TW_DATA} DESTINATION "${CMAKE_INSTALL_PREFIX}/data")
    install(FILES ${TW_RES}  DESTINATION "${CMAKE_INSTALL_PREFIX}/res")
    install(FILES ${TW_DOCS} DESTINATION "${CMAKE_INSTALL_PREFIX}/docs")
    install(DIRECTORY ${TW_CCLP_DOCS} DESTINATION "${CMAKE_INSTALL_PREFIX}/docs")
else()
    set(SHARE_DIR "${CMAKE_INSTALL_PREFIX}/share/tworld" CACHE STRING
        "Directory for shared files")
    add_definitions(-Dstricmp=strcasecmp)

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_definitions(-DROOTDIR="${SHARE_DIR}")
    else()
        add_definitions(-DSAVEDIR="./save")
    endif()

    install(FILES ${TW_SETS} DESTINATION "${SHARE_DIR}/sets")
    install(FILES ${TW_DATA} DESTINATION "${SHARE_DIR}/data")
    install(FILES ${TW_RES}  DESTINATION "${SHARE_DIR}/res")
    install(FILES ${TW_MAN6} DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man6")
    install(DIRECTORY ${TW_CCLP_DOCS} DESTINATION "${SHARE_DIR}/docs")
endif()

add_subdirectory(oshw-${OSHW})

add_executable(${TWORLD_EXE} WIN32 MACOSX_BUNDLE tworld.c)
target_sources(${TWORLD_EXE} PRIVATE
    cmdline.h
    cmdline.c
    comptime.h
    defs.h
    encoding.h
    encoding.c
    err.h
    err.c
    fileio.h
    fileio.c
    gen.h
    help.h
    help.c
    logic.h
    lxlogic.c
    messages.h
    messages.cpp
    mslogic.c
    oshw.h
    play.h
    play.c
    random.h
    random.c
    res.h
    res.c
    score.h
    score.cpp
    series.h
    series.c
    settings.h
    settings.cpp
    solution.h
    solution.c
    state.h
    unslist.h
    unslist.c
    ver.h
)

if(WIN32)
    target_sources(${TWORLD_EXE} PRIVATE ${TWORLD_EXE}.rc)
endif()

if(OSHW STREQUAL "qt")
    target_sources(${TWORLD_EXE} PRIVATE tworld2.qrc)
endif()


target_link_libraries(${TWORLD_EXE} PRIVATE oshw-${OSHW})
if(WIN32)
    target_link_libraries(${TWORLD_EXE} PRIVATE $<$<CONFIG:Debug>:-mconsole>)
endif()

if(OSHW STREQUAL "qt")
    target_link_libraries(${TWORLD_EXE} PRIVATE SDL2::SDL2)
endif()

# Generate the current compile time
if(WIN32)
    # TODO: It would be nice to use the format below on Win32 as well...
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/comptime.bat"
         "echo #define COMPILE_TIME \"%DATE% %TIME%\" > comptime.h")
    add_custom_command(
        OUTPUT comptime.h
        COMMAND comptime.bat
    )
else()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/comptime.sh"
         "echo \"#define COMPILE_TIME \\\"`date '+%Y %b %e %T %Z'`\\\"\" > comptime.h")
    add_custom_command(
        OUTPUT comptime.h
        COMMAND /bin/sh comptime.sh
    )
endif()

set_property(SOURCE comptime.h PROPERTY SKIP_AUTOGEN ON)
target_include_directories(${TWORLD_EXE} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

if(WIN32 OR APPLE)
    install(TARGETS ${TWORLD_EXE}
            RUNTIME DESTINATION .
            BUNDLE DESTINATION .
    )
else()
    install(TARGETS ${TWORLD_EXE} RUNTIME DESTINATION bin)
endif()

add_executable(mklynxcc EXCLUDE_FROM_ALL mklynxcc.c)
