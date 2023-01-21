#!/bin/bash -e
#
# Build The Great Escape for RISC OS via GCCSDK
#

BUILDDIR=build-ro

if ! command -v cmake &>/dev/null; then
	echo "CMake could not be found"
	exit
fi

if command -v ninja &>/dev/null; then
	GENERATOR="-G Ninja"
	PARALLEL=
else
	GENERATOR=
	PARALLEL="--parallel 5" # wild guess
fi

mkdir -p $BUILDDIR && cd $BUILDDIR
echo "Configuring..."
cmake $GENERATOR -DCMAKE_TOOLCHAIN_FILE==${APPENGINE_ROOT}/cmake/riscos.cmake ..
echo "Building..."
cmake --build . $PARALLEL
echo "Installing..."
cmake --install .
