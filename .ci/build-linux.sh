#!/bin/sh -ex

# Setup Qt variables
export QT_BASE_DIR=/opt/qt${QTVERMIN}
export PATH=$QT_BASE_DIR/bin:$PATH
export LD_LIBRARY_PATH=$QT_BASE_DIR/lib/x86_64-linux-gnu:$QT_BASE_DIR/lib

cd rpcs3 || exit 1

# Pull all the submodules except llvm, since it is built separately and we just download that build
# Note: Tried to use git submodule status, but it takes over 20 seconds
# shellcheck disable=SC2046
git submodule -q update --init $(awk '/path/ && !/llvm/ { print $3 }' .gitmodules)

# Download pre-compiled llvm libs
curl -sLO https://github.com/RPCS3/llvm-mirror/releases/download/custom-build/llvmlibs-linux.tar.gz
mkdir llvmlibs
tar -xzf ./llvmlibs-linux.tar.gz -C llvmlibs

mkdir build && cd build || exit 1

if [ "$COMPILER" = "gcc" ]; then
    # These are set in the dockerfile
    export CC=${GCC_BINARY}
    export CXX=${GXX_BINARY}
    export LINKER=gold
    # We need to set the following variables for LTO to link properly
    export AR=/usr/bin/gcc-ar-$GCCVER
    export RANLIB=/usr/bin/gcc-ranlib-$GCCVER
    export CFLAGS="-fuse-linker-plugin"
else
    export CC=${CLANG_BINARY}
    export CXX=${CLANGXX_BINARY}
    export LINKER=lld
    export AR=/usr/bin/llvm-ar-$LLVMVER
    export RANLIB=/usr/bin/llvm-ranlib-$LLVMVER
fi

export CFLAGS="$CFLAGS -fuse-ld=${LINKER}"

cmake ..                                               \
    -DCMAKE_INSTALL_PREFIX=/usr                        \
    -DBUILD_LLVM_SUBMODULE=OFF                         \
    -DLLVM_DIR=llvmlibs/lib/cmake/llvm/                \
    -DUSE_NATIVE_INSTRUCTIONS=OFF                      \
    -DUSE_PRECOMPILED_HEADERS=OFF                      \
    -DCMAKE_C_FLAGS="$CFLAGS"                          \
    -DCMAKE_CXX_FLAGS="$CFLAGS"                        \
    -DCMAKE_AR="$AR"                                   \
    -DCMAKE_RANLIB="$RANLIB"                           \
    -DUSE_SYSTEM_CURL=ON                               \
    -G Ninja

ninja; build_status=$?;

cd ..

# If it compiled succesfully let's deploy depending on the build pipeline (Travis, Azure Pipelines).
# Travis only deploys on master, and it publishes to GitHub releases. Azure publishes PRs as artifacts
# only.
{   [ "$IS_AZURE" = "true" ] ||
    { [ "$TRAVIS_BRANCH" = "master" ] && [ "$TRAVIS_PULL_REQUEST" = "false" ]; };
} && SHOULD_DEPLOY="true" || SHOULD_DEPLOY="false"

if [ "$build_status" -eq 0 ] && [ "$SHOULD_DEPLOY" = "true" ]; then
    .ci/deploy-linux.sh
fi
