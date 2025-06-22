#!/bin/sh -ex

git config --global --add safe.directory '*'

# Pull all the submodules except some
# Note: Tried to use git submodule status, but it takes over 20 seconds
# shellcheck disable=SC2046
git submodule -q update --init $(awk '/path/ && !/llvm/ && !/opencv/ && !/ffmpeg/ && !/curl/ && !/FAudio/ && !/zlib/ { print $3 }' .gitmodules)

mkdir build && cd build || exit 1

export CC="clang"
export CXX="clang++"
export LINKER=lld
export LINKER_FLAG="-fuse-ld=${LINKER}"

if [ -n "$LLVMVER" ]; then
  export AR="llvm-ar-$LLVMVER"
  export RANLIB="llvm-ranlib-$LLVMVER"
else
  export AR="llvm-ar"
  export RANLIB="llvm-ranlib"
fi

cmake ..                                               \
    -DCMAKE_PREFIX_PATH=/clang64                       \
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
    -DUSE_FAUDIO=OFF                                   \
    -DUSE_SDL=ON                                       \
    -DUSE_SYSTEM_SDL=OFF                               \
    -DUSE_SYSTEM_FFMPEG=ON                             \
    -DUSE_SYSTEM_OPENCV=ON                             \
    -DUSE_SYSTEM_OPENAL=OFF                            \
    -DUSE_DISCORD_RPC=ON                               \
    -DOpenGL_GL_PREFERENCE=LEGACY                      \
    -DWITH_LLVM=ON                                     \
    -DLLVM_DIR=/clang64/lib/cmake/llvm                 \
    -DVulkan_LIBRARY=/clang64/lib/libvulkan-1.dll.a    \
    -DSTATIC_LINK_LLVM=ON                              \
    -DBUILD_RPCS3_TESTS=OFF                            \
    -DRUN_RPCS3_TESTS=OFF                              \
    -G Ninja

ninja; build_status=$?;

cd ..

# If it compiled succesfully let's deploy.
if [ "$build_status" -eq 0 ]; then
    .ci/deploy-windows-clang.sh "x86_64"
fi
