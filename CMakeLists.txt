# CMakeLists.txt
#
# The Great Escape
#
# Copyright (c) David Thomas, 2017-2018
#

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

project(TheGreatEscape)


set(DESCRIPTION "The Great Escape")
set(AUTHOR "David Thomas")
set(EMAIL dave@davespace.co.uk)
set(CODE_IDENTIFIER uk.co.davespace.TheGreatEscape)
#set(CODE_SIGN_IDENTITY )
set(VERSION 1.0)
set(COPYRIGHT "Copyright 1986 Ocean Software Ltd. (The Great Escape). Copyright 2012-2018 David Thomas &lt;dave@davespace.co.uk&gt; (this version).")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")


set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")


add_subdirectory(libraries/ZXSpectrum)
add_subdirectory(libraries/TheGreatEscape)

if(WIN32)
  add_subdirectory(platform/windows)
elseif(APPLE)
  add_subdirectory(platform/osx)
else()
  add_subdirectory(platform/generic)
endif()
