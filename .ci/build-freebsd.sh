#!/bin/sh -ex

# Pull all the submodules except some
# Note: Tried to use git submodule status, but it takes over 20 seconds
# shellcheck disable=SC2046
git config --global --add safe.directory .
git submodule -q update --init --depth 1 $(awk '/path/ && !/llvm/ && !/opencv/ && !/libpng/ && !/libsdl-org/ && !/curl/ && !/zlib/ && !/libusb/ && !/feralinteractive/ { print $3 }' .gitmodules)

CONFIGURE_ARGS="
	-DWITH_LLVM=ON
	-DUSE_SDL=OFF
	-DUSE_PRECOMPILED_HEADERS=OFF
	-DUSE_NATIVE_INSTRUCTIONS=OFF
	-DUSE_SYSTEM_FFMPEG=ON
	-DUSE_SYSTEM_CURL=ON
	-DUSE_SYSTEM_LIBPNG=ON
 	-DUSE_SYSTEM_LIBUSB=ON
	-DUSE_SYSTEM_OPENCV=ON
"

# base Clang workaround (missing clang-scan-deps)
CONFIGURE_ARGS="$CONFIGURE_ARGS -DCMAKE_CXX_SCAN_FOR_MODULES=OFF"

# shellcheck disable=SC2086
cmake -B build -G Ninja $CONFIGURE_ARGS
cmake --build build

ccache --show-stats
ccache --zero-stats
