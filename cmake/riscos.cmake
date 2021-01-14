# CMake toolchain file for RISC OS GCCSDK

set(tools $ENV{GCCSDK_INSTALL_CROSSBIN})
set(CMAKE_C_COMPILER ${tools}/arm-unknown-riscos-gcc)
set(CMAKE_CXX_COMPILER ${tools}/arm-unknown-riscos-g++)

set(CMAKE_C_FLAGS "-std=c99 -mlibscl -mhard-float" CACHE STRING "" FORCE)
# I would prefer to use -Os for size but that made GCCSDK explode when I tried it.
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG -march=armv2 -mtune=arm2 -mno-poke-function-name" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG "-mpoke-function-name" CACHE STRING "" FORCE)

set(CMAKE_FIND_ROOT_PATH ${tools})

# Search for programs only in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# Search for libraries and headers only in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(TARGET_RISCOS 1)
