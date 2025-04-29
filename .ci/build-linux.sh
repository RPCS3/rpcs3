#!/bin/sh -ex

if [ -z "$CIRRUS_CI" ]; then
   cd rpcs3 || exit 1
fi

shellcheck .ci/*.sh

RPCS3_DIR=$(pwd)

# If we're building using a CI, let's use the runner's directory
if [ -n "$BUILDDIR" ]; then
BUILD_DIR="$BUILDDIR"
else
BUILD_DIR="$RPCS3_DIR/build"
fi

git config --global --add safe.directory '*'

# Pull all the submodules except llvm, opencv, sdl and curl
# Note: Tried to use git submodule status, but it takes over 20 seconds
# shellcheck disable=SC2046
git submodule -q update --init $(awk '/path/ && !/llvm/ && !/opencv/ && !/libsdl-org/ && !/curl/ { print $3 }' .gitmodules)

if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR" || exit 1
fi

cd "$BUILD_DIR" || exit 1

if [ "$COMPILER" = "gcc" ]; then
    # These are set in the dockerfile
    export CC="${GCC_BINARY}"
    export CXX="${GXX_BINARY}"
    export LINKER=gold
    # We need to set the following variables for LTO to link properly
    export AR=/usr/bin/gcc-ar-"$GCCVER"
    export RANLIB=/usr/bin/gcc-ranlib-"$GCCVER"
    export CFLAGS="-fuse-linker-plugin"
else
    export CC="${CLANG_BINARY}"
    export CXX="${CLANGXX_BINARY}"
    export LINKER=lld
    export AR=/usr/bin/llvm-ar-"$LLVMVER"
    export RANLIB=/usr/bin/llvm-ranlib-"$LLVMVER"
fi

export LINKER_FLAG="-fuse-ld=${LINKER}"

cmake "$RPCS3_DIR"                                     \
    -DCMAKE_INSTALL_PREFIX=/usr                        \
    -DUSE_NATIVE_INSTRUCTIONS=OFF                      \
    -DUSE_PRECOMPILED_HEADERS=OFF                      \
    -DCMAKE_C_FLAGS="$CFLAGS"                          \
    -DCMAKE_CXX_FLAGS="$CFLAGS"                        \
    -DCMAKE_EXE_LINKER_FLAGS="${LINKER_FLAG}"          \
    -DCMAKE_MODULE_LINKER_FLAGS="${LINKER_FLAG}"       \
    -DCMAKE_SHARED_LINKER_FLAGS="${LINKER_FLAG}"       \
    -DCMAKE_AR="$AR"                                   \
    -DCMAKE_RANLIB="$RANLIB"                           \
    -DUSE_SYSTEM_CURL=ON                               \
    -DUSE_SDL=ON                                       \
    -DUSE_SYSTEM_SDL=ON                                \
    -DUSE_SYSTEM_FFMPEG=OFF                            \
    -DUSE_SYSTEM_OPENCV=ON                             \
    -DUSE_DISCORD_RPC=ON                               \
    -DOpenGL_GL_PREFERENCE=LEGACY                      \
    -DLLVM_DIR=/opt/llvm/lib/cmake/llvm                \
    -DSTATIC_LINK_LLVM=ON                              \
    -DBUILD_RPCS3_TESTS="${BUILD_UNIT_TESTS}"          \
    -DRUN_RPCS3_TESTS="${RUN_UNIT_TESTS}"              \
    -G Ninja

ninja; build_status=$?;

cd "$RPCS3_DIR"

# If it compiled succesfully let's deploy.
# Azure and Cirrus publish PRs as artifacts only.
# {   [ "$CI_HAS_ARTIFACTS" = "true" ];
# } && SHOULD_DEPLOY="true" || SHOULD_DEPLOY="false"

if [ "$build_status" -eq 0 ] && [ "$SHOULD_DEPLOY" = "true" ]; then
    echo "deploy-linux placeholder"
    # .ci/deploy-linux.sh "x86_64"
fi
