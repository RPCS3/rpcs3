#!/bin/sh -ex

# Pull all the submodules except llvm
# Note: Tried to use git submodule status, but it takes over 20 seconds
# shellcheck disable=SC2046
git submodule -q update --init --depth 1 $(awk '/path/ && !/llvm/ { print $3 }' .gitmodules)

# Prefer newer Clang on TravisCI (see also .ci/install-freebsd.sh)
if [ -n "$TRAVIS" ]; then
	fetch https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/libcxx-11.0.0.src.tar.xz
	tar xf libcxx-11.0.0.src.tar.xz
	export CC=clang11 CXX=clang++11
	export CXXFLAGS="$CXXFLAGS -nostdinc++ -isystem $PWD/libcxx-11.0.0.src/include"
fi

CONFIGURE_ARGS="
	-DWITH_LLVM=OFF
	-DUSE_PRECOMPILED_HEADERS=OFF
	-DUSE_NATIVE_INSTRUCTIONS=OFF
	-DUSE_SYSTEM_FFMPEG=ON
	-DUSE_SYSTEM_CURL=ON
	-DUSE_SYSTEM_LIBPNG=ON
"

# shellcheck disable=SC2086
cmake -B build -G Ninja $CONFIGURE_ARGS
cmake --build build

ccache --show-stats
ccache --zero-stats
