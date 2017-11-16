cmake_minimum_required(VERSION 2.8)

project(TheGreatEscape)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")



set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
#" CACHE STRING "" FORCE)



add_subdirectory(libraries/ZXSpectrum)
add_subdirectory(libraries/TheGreatEscape)

# if generic build...
add_subdirectory(platform/osx)
