# CMakeLists.txt
#
# The Great Escape in C
#
# Copyright (c) David Thomas, 2021
#
# vim: sw=4 ts=8 et

cmake_minimum_required(VERSION 3.18)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

set(APPENGINE_REPO_ROOT $ENV{APPENGINE_ROOT})

list(APPEND CMAKE_MODULE_PATH ${APPENGINE_REPO_ROOT}/cmake/Modules)

include(FetchContent)

project(GtEscape VERSION 0.9.3 DESCRIPTION "Escape game" LANGUAGES C)

# If a host tools build was requested, then build them alone
set(TEMPLHEADR_DIR ${APPENGINE_REPO_ROOT}/utils/templheader) # Note: Dir has an incorrect extra 'e'
if(HOST_TOOLS_ONLY)
    message(STATUS "${PROJECT_NAME}: Building host tools only")
    add_subdirectory(${TEMPLHEADR_DIR} templheadr-build)
    return()
endif()

# The values set in the toolchain file aren't available until this point.
if(NOT DEFINED TARGET_RISCOS)
    message(FATAL_ERROR "${PROJECT_NAME} builds for RISC OS only")
endif()

if(TARGET_RISCOS)
    riscos_set_flags()
endif()

# Referencing CMAKE_TOOLCHAIN_FILE avoids a warning on rebuilds.
if(NOT ${CMAKE_TOOLCHAIN_FILE} STREQUAL "")
    message(STATUS "${PROJECT_NAME}: Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Build the native 'host' tools (...) in this dir
set(HOSTTOOLSDIR "${CMAKE_BINARY_DIR}/host")

# Create a directory in which to build the host tools
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${HOSTTOOLSDIR}")

# Generate the host tools build
#
# We use -DCMAKE_C_COMPILER to force the use of the system compiler, otherwise
# the child build may inherit a cross-compiler via the CC environment variable.
# This is bound to break on Windows.
execute_process(COMMAND ${CMAKE_COMMAND}
    -G "${CMAKE_GENERATOR}"
    -DCMAKE_BUILD_TYPE='Release'
    -DCMAKE_C_COMPILER='cc'
    -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
    -DHOST_TOOLS_ONLY=1
    ${CMAKE_SOURCE_DIR}
    WORKING_DIRECTORY "${HOSTTOOLSDIR}")

# Build the host tools (by recursing)
execute_process(COMMAND ${CMAKE_COMMAND} --build . --target templheadr WORKING_DIRECTORY "${HOSTTOOLSDIR}")

# Build GtEscape itself
message(STATUS "${PROJECT_NAME}: Building targets only")

# Libs... (set this before imports)
#
set(LIBS_DIR ${APPENGINE_REPO_ROOT}/libs)

# Import DPTLib
FetchContent_Declare(DPTLib
    GIT_REPOSITORY https://github.com/dpt/DPTLib
    GIT_TAG 0.3.0)
# Expanding out FetchContent_MakeAvailable(DPTLib) here so I can use EXCLUDE_FROM_ALL
FetchContent_GetProperties(DPTLib)
if(NOT DPTLib_POPULATED)
    FetchContent_Populate(DPTLib)
    add_subdirectory(${dptlib_SOURCE_DIR} ${dptlib_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Import AppEngine
set(APPENGINE_DIR ${LIBS_DIR}/appengine)
set(APPENGINE_INCLUDE ${APPENGINE_DIR}/../)
add_subdirectory(${APPENGINE_DIR} appengine-build EXCLUDE_FROM_ALL)


# Avoid a warning from CMake
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

add_executable(GtEscape)

target_include_directories(GtEscape
    PRIVATE
    ${APPENGINE_INCLUDE}
    ${DPTLIB_INCLUDE}
    ${CMAKE_SOURCE_DIR})

target_link_libraries(GtEscape PRIVATE AppEngine DPTLib)

set_target_properties(GtEscape PROPERTIES
    DESCRIPTION "Game"
    C_STANDARD 99
    RUNTIME_OUTPUT_NAME !RunImage)

set(ALL_SOURCES
    globals.c
    globals.h
    iconbar.c
    iconbar.h
    main.c
    menunames.h
    poll.c
    poll.h
    ssbuffer.h
    timermod.c
    timermod.h
    zxgame.c
    zxgame.h
    zxgames.c
    zxgames.h
    zxscale.c
    zxscale.h)

target_sources(GtEscape PRIVATE ${ALL_SOURCES})


# Generate iconnames.h
#

set(TEMPLATES ${CMAKE_CURRENT_SOURCE_DIR}/Resources/UK/Templates,fec)
set(ICONNAMES_H ${CMAKE_CURRENT_BINARY_DIR}/iconnames.h)

# Note: We have to look for templheadr-build relative to where it's built.
find_program(TEMPLHEADR
    templheadr
    PATHS ${HOSTTOOLSDIR}/platform/riscos/!GtEscape/templheadr-build
    PATH_SUFFIXES Debug Release RelWithDebInfo MinSizeRel
    NO_DEFAULT_PATH
    REQUIRED)
add_custom_command(OUTPUT ${ICONNAMES_H}
    COMMAND ${TEMPLHEADR} ${TEMPLATES} ${ICONNAMES_H}
    MAIN_DEPENDENCY ${TEMPLATES})
add_custom_target(GtEscapeIconNames ALL DEPENDS ${ICONNAMES_H})

add_dependencies(GtEscape GtEscapeIconNames)

target_include_directories(GtEscape PRIVATE ${CMAKE_CURRENT_BINARY_DIR})


# Build options
#

target_compile_options(GtEscape PRIVATE -Wall -Wextra -pedantic)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(GtEscape PRIVATE GTESCAPE_DEBUG)
endif()

# Library dependencies
#

target_link_libraries(GtEscape PRIVATE ZXSpectrum TheGreatEscape)

if(TARGET_RISCOS)
    # OSLib (& anything else in GCCSDK)
    target_include_directories(GtEscape PRIVATE $ENV{GCCSDK_INSTALL_ENV}/include)
    target_link_libraries(GtEscape PRIVATE $ENV{GCCSDK_INSTALL_ENV}/lib/libOSLib32.a)
endif()

option(USE_FORTIFY "Use Fortify" OFF)
if(USE_FORTIFY)
    target_link_libraries(GtEscape PRIVATE Fortify)
endif()

# Feature options
#

if(TGE_SAVES)
    target_compile_definitions(GtEscape PRIVATE TGE_SAVES)
    target_sources(GtEscape PRIVATE dataxfer.c dataxfer.h zxsave.c zxsave.h)
endif()

# Installation
#

# (force this for now)
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/install" CACHE PATH "CMake install prefix" FORCE)
set(INSTALL_APPDIR ${CMAKE_INSTALL_PREFIX}/Apps/Games/!GtEscape)

install(TARGETS GtEscape RUNTIME
    DESTINATION ${INSTALL_APPDIR})
install(FILES !Boot,feb !Help,feb !Run,feb !Sprites,ff9 !Sprites22,ff9 AppEngine,ffa TimerMod,ffa
    DESTINATION ${INSTALL_APPDIR})
install(DIRECTORY Resources
    DESTINATION ${INSTALL_APPDIR})
install(DIRECTORY ../Package
    DESTINATION ${CMAKE_INSTALL_PREFIX})
# this release stuff needs reorganising for RiscPkg
install(DIRECTORY ../Release
    DESTINATION ${CMAKE_INSTALL_PREFIX})

