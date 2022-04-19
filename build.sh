#!/bin/bash -e
#
# Build The Great Escape natively
#

BUILDDIR=build

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
cmake $GENERATOR ..
echo "Building..."
cmake --build . $PARALLEL
