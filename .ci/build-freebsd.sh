#!/bin/sh -ex

# Pull all the submodules except llvm
# Note: Tried to use git submodule status, but it takes over 20 seconds
# shellcheck disable=SC2046
git submodule -q update --init --depth 1 $(awk '/path/ && !/llvm/ { print $3 }' .gitmodules)

# Prefer newer Clang than in base system (see also .ci/install-freebsd.sh)
# libc++ isn't in llvm* packages, so download manually
fetch https://github.com/llvm/llvm-project/archive/refs/tags/llvmorg-13.0.1-rc1.tar.gz
tar xf llvm*.tar.gz
export CC=clang13 CXX=clang++13
cmake -B libcxx_build -G Ninja -S llvm*/libcxx \
      -DLLVM_CCACHE_BUILD=ON \
      -DCMAKE_INSTALL_PREFIX:PATH=libcxx_prefix
cmake --build libcxx_build
cmake --install libcxx_build
export CXXFLAGS="$CXXFLAGS -nostdinc++ -isystem$PWD/libcxx_prefix/include/c++/v1"
export LDFLAGS="$LDFLAGS -nostdlib++ -L$PWD/libcxx_prefix/lib -l:libc++.a -lcxxrt"

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
