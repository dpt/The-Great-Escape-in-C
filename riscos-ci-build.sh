#!/bin/bash
##
# Build RISC OS 'The Great Escape' pulling in all resources.
#
# Run me with:
#    docker run -it --rm -v "$PWD:/work" -w /work riscosdotinfo/riscos-gccsdk-4.7:latest ./riscos-ci-build.sh

set -e
set -o pipefail

# Building OSLib will be super slow.
BUILD_OSLIB=false

scriptdir="$(cd "$(dirname "$0")" && pwd -P)"

wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | apt-key add -
cmake --version || { apt-get update && \
                     apt-get install -y software-properties-common && \
                     apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main' && \
                     apt-get install -y cmake ; }

ninja --version || { apt-get update && \
                     apt-get install -y ninja-build ; }

rsync --version || { apt-get update && \
                     apt-get install -y rsync ; }

rename --version || { apt-get update && \
                     apt-get install -y rename ; }

source /home/riscos/gccsdk-params

# AppEngine
if [[ ! -d PrivateEye ]] ; then
    git clone https://github.com/dpt/PrivateEye
    cd PrivateEye
fi
export APPENGINE_ROOT=${scriptdir}/PrivateEye

cd $APPENGINE_ROOT

if [[ ! -d PrivateEye/libs/flex ]] ; then
    cd libs
    ./grabflex.sh
    cd ..
fi

if $BUILD_OSLIB ; then
    if [[ ! -d libs/oslib || ! -d "$GCCSDK_INSTALL_ENV/include/oslib/" ]] ; then
        svn co 'svn://svn.code.sf.net/p/ro-oslib/code/trunk/!OSLib' oslib
        cd oslib
        make ELFOBJECTTYPE=HARDFPU install
        cd ..
    fi
else
    if [[ ! -d libs/oslib-bin || ! -d "$GCCSDK_INSTALL_ENV/include/oslib/" ]] ; then
        mkdir -p oslib-bin
        cd oslib-bin
        if [[ ! -f oslib.zip ]] ; then
            wget -O oslib.zip 'https://sourceforge.net/projects/ro-oslib/files/OSLib/7.00/OSLib-elf-scl-app-hardfloat-7.00.zip/download?use_mirror=master'
            unzip oslib.zip
        fi
        mkdir -p "$GCCSDK_INSTALL_ENV/include/oslib/"
        mkdir -p "$GCCSDK_INSTALL_ENV/lib/"
        rename 's!/h/(.*)$!/$1.h!' oslib/h/*
        rsync -av oslib/ "$GCCSDK_INSTALL_ENV/include/oslib/"
        cp libOSLib32.a "$GCCSDK_INSTALL_ENV/lib/"
        cd ../../
    fi
fi

cd $scriptdir

# The Great Escape
mkdir -p build-riscos && cd build-riscos
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=${APPENGINE_ROOT}/cmake/riscos.cmake .. || bash -i
ninja install || bash -i
