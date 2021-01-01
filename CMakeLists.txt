# CMakeLists.txt
#
# The Great Escape
#
# Copyright (c) David Thomas, 2017-2020
#

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(ExternalProject)


project(TheGreatEscape)

set(DESCRIPTION "The Great Escape")
set(AUTHOR "David Thomas")
set(EMAIL dave@davespace.co.uk)
set(CODE_IDENTIFIER uk.co.davespace.TheGreatEscape)
#set(CODE_SIGN_IDENTITY )
set(VERSION 1.0)
set(COPYRIGHT "Copyright 1986 Ocean Software Ltd. (The Great Escape). Copyright 2012-2020 David Thomas &lt;dave@davespace.co.uk&gt; (this version).")


option(TGE_SAVES "Enable loading and saving of games" ON)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")

set_property(GLOBAL PROPERTY C_STANDARD 90)

if(CMAKE_BUILD_TYPE MATCHES MinSizeRel)
    # Use -Oz instead of -Os
    set(CMAKE_C_FLAGS_MINSIZEREL "-Oz -DNDEBUG") # should be macOS only!
endif()


if(TGE_SAVES)
  set(ZEROTAPE_INSTALL_DIR ${CMAKE_BINARY_DIR}/zerotape)
  ExternalProject_Add(zerotape
      GIT_REPOSITORY /Users/dave/SyncProjects/github/zerotape
      GIT_TAG 0.4.0
      CMAKE_ARGS
          -DCMAKE_INSTALL_PREFIX=${ZEROTAPE_INSTALL_DIR}
          -DCMAKE_BUILD_TYPE=Release)
  set(ZEROTAPE_INCLUDE ${ZEROTAPE_INSTALL_DIR}/include)
  set(ZEROTAPE_LIB ${ZEROTAPE_INSTALL_DIR}/lib/libzerotape.a)
endif()


add_subdirectory(libraries/ZXSpectrum)
add_subdirectory(libraries/TheGreatEscape)

if(WIN32)
  add_subdirectory(platform/windows)
elseif(APPLE)
  add_subdirectory(platform/osx)
else()
  add_subdirectory(platform/generic)
endif()
