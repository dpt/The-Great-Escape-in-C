# CMakeLists.txt
#
# The Great Escape in C
#
# Copyright (c) David Thomas, 2017-2024
#
# vim: sw=4 ts=8 et

# When the RISC OS build recurses, in order to build host tools, it will enter
# here. That won't naturally fall through to the right code, so divert here.
if(HOST_TOOLS_ONLY)
    add_subdirectory(platform/riscos/!GtEscape)
    return()
endif()

cmake_minimum_required(VERSION 3.18)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(ExternalProject)


project(TheGreatEscape VERSION 1.0.0 DESCRIPTION "The Great Escape" LANGUAGES C CXX)

set(DESCRIPTION "The Great Escape")
set(AUTHOR "David Thomas")
set(EMAIL dave@davespace.co.uk)
set(CODE_IDENTIFIER uk.co.davespace.TheGreatEscape)
#set(CODE_SIGN_IDENTITY )
set(VERSION 1.0.0)
set(COPYRIGHT "Copyright 1986 Ocean Software Ltd. (The Great Escape). Copyright 2012-2024 David Thomas &lt;dave@davespace.co.uk&gt; (this version).")


option(TGE_SAVES "Enable loading and saving of games" ON)


find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
endif()

set_property(GLOBAL PROPERTY C_STANDARD 90)

# Where -Oz is supported, use it instead of -Os (smaller)
if(APPLE AND CMAKE_BUILD_TYPE MATCHES MinSizeRel)
    set(CMAKE_C_FLAGS_MINSIZEREL "-Oz -DNDEBUG")
endif()

# Referencing CMAKE_TOOLCHAIN_FILE avoids a warning on rebuilds
if(NOT ${CMAKE_TOOLCHAIN_FILE} STREQUAL "")
    message(STATUS "The Great Escape: Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
endif()



if(TGE_SAVES)
    set(ZEROTAPE_INSTALL_DIR ${CMAKE_BINARY_DIR}/zerotape)
    set(ZEROTAPE_INCLUDE ${ZEROTAPE_INSTALL_DIR}/include)
    if(MSVC)
        set(ZEROTAPE_LIB ${ZEROTAPE_INSTALL_DIR}/lib/zerotape.lib)
    else()
        set(ZEROTAPE_LIB ${ZEROTAPE_INSTALL_DIR}/lib/libzerotape.a)
    endif()
    # BUILD_BYPRODUCTS must be specified in this command otherwise ninja won't
    # know when to build zerotape.
    ExternalProject_Add(zerotape
        GIT_REPOSITORY https://github.com/dpt/zerotape
        GIT_TAG 0.6.0
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${ZEROTAPE_INSTALL_DIR} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        BUILD_BYPRODUCTS ${ZEROTAPE_LIB})
endif()


add_subdirectory(libraries/ZXSpectrum)
add_subdirectory(libraries/TheGreatEscape)

if(APPLE)
    add_subdirectory(platform/osx)
elseif(TARGET_RISCOS)
    add_subdirectory(platform/riscos/!GtEscape)
elseif(MSVC)
    add_subdirectory(platform/windows)
else()
    add_subdirectory(platform/generic)
endif()
