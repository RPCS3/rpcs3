#!/bin/env bash -ex
shopt -s nocasematch

# Setup Qt variables
export QT_BASE_DIR=/opt/qt${QTVERMIN}
export PATH=$QT_BASE_DIR/bin:$PATH
export LD_LIBRARY_PATH=$QT_BASE_DIR/lib/x86_64-linux-gnu:$QT_BASE_DIR/lib

cd rpcs3

git submodule update --quiet --init asmjit 3rdparty/ffmpeg 3rdparty/pugixml 3rdparty/span 3rdparty/libpng 3rdparty/cereal 3rdparty/hidapi 3rdparty/xxHash 3rdparty/yaml-cpp 3rdparty/libusb 3rdparty/FAudio Vulkan/glslang 3rdparty/curl 3rdparty/wolfssl

# Download pre-compiled llvm libs
curl -sLO https://github.com/RPCS3/llvm-mirror/releases/download/custom-build/llvmlibs-linux.tar.gz
mkdir llvmlibs
tar -xzf ./llvmlibs-linux.tar.gz -C llvmlibs

mkdir build ; cd build

if [ $COMPILER = "gcc" ]; then
	# These are set in the dockerfile
	export CC=${GCC_BINARY}
	export CXX=${GXX_BINARY}
	export LINKER=gold
	# We need to set the following variables for LTO to link properly
	export AR=/usr/bin/gcc-ar-9
	export RANLIB=/usr/bin/gcc-ranlib-9
	export CFLAGS="-fuse-linker-plugin"
else
	export CC=${CLANG_BINARY}
	export CXX=${CLANGXX_BINARY}
	export LINKER=lld
	export AR=/usr/bin/llvm-ar-$LLVMVER
	export RANLIB=/usr/bin/llvm-ranlib-$LLVMVER
fi

export CFLAGS="$CFLAGS -fuse-ld=${LINKER}"

cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_LLVM_SUBMODULE=OFF -DUSE_COTIRE=OFF -DLLVM_DIR=llvmlibs/lib/cmake/llvm/ -DUSE_NATIVE_INSTRUCTIONS=OFF \
  -DCMAKE_C_FLAGS="$CFLAGS" -DCMAKE_CXX_FLAGS="$CFLAGS" -DCMAKE_AR=$AR -DCMAKE_RANLIB=$RANLIB \
  -G Ninja

ninja; build_status=$?;

cd ..

# If it compiled succesfully let's deploy depending on the build pipeline (Travis, Azure Pipelines)
# BUILD_REASON is an Azure Pipeline variable, and we want to deploy when using Azure Pipelines
if [[ $build_status -eq 0 && ( -n "$BUILD_REASON" || ( "$TRAVIS_BRANCH" = "master" && "$TRAVIS_PULL_REQUEST" = false ) ) ]]; then
	/bin/bash -ex .travis/deploy-linux.bash
fi
