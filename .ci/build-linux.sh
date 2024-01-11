#!/bin/sh -ex

if [ -z "$CIRRUS_CI" ]; then
   cd rpcs3 || exit 1
fi

git config --global --add safe.directory '*'

# Pull all the submodules except llvm
# Note: Tried to use git submodule status, but it takes over 20 seconds
# shellcheck disable=SC2046
git submodule -q update --init $(awk '/path/ && !/llvm/ && !/SPIRV/ { print $3 }' .gitmodules)

mkdir build && cd build || exit 1

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

export CFLAGS="$CFLAGS -fuse-ld=${LINKER}"

cmake ..                                               \
    -DCMAKE_INSTALL_PREFIX=/usr                        \
    -DUSE_NATIVE_INSTRUCTIONS=OFF                      \
    -DUSE_PRECOMPILED_HEADERS=OFF                      \
    -DCMAKE_C_FLAGS="$CFLAGS"                          \
    -DCMAKE_CXX_FLAGS="$CFLAGS"                        \
    -DCMAKE_AR="$AR"                                   \
    -DCMAKE_RANLIB="$RANLIB"                           \
    -DUSE_SYSTEM_CURL=ON                               \
    -DUSE_SDL=ON                                       \
    -DUSE_SYSTEM_FFMPEG=OFF                            \
    -DOpenGL_GL_PREFERENCE=LEGACY                      \
    -DLLVM_DIR=/opt/llvm/lib/cmake/llvm                \
    -DSTATIC_LINK_LLVM=ON                              \
    -G Ninja

ninja; build_status=$?;

cd ..

shellcheck .ci/*.sh

# If it compiled succesfully let's deploy.
# Azure and Cirrus publish PRs as artifacts only.
{   [ "$CI_HAS_ARTIFACTS" = "true" ];
} && SHOULD_DEPLOY="true" || SHOULD_DEPLOY="false"

if [ "$build_status" -eq 0 ] && [ "$SHOULD_DEPLOY" = "true" ]; then
    .ci/deploy-linux.sh
fi
