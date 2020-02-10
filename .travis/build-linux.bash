#!/bin/env bash -ex
shopt -s nocasematch

# Setup Qt variables
export QT_BASE_DIR=/opt/qt${QTVERMIN}
export PATH=$QT_BASE_DIR/bin:$PATH
export LD_LIBRARY_PATH=$QT_BASE_DIR/lib/x86_64-linux-gnu:$QT_BASE_DIR/lib

cd rpcs3

git submodule update --quiet --init asmjit 3rdparty/ffmpeg 3rdparty/pugixml 3rdparty/span 3rdparty/libpng 3rdparty/cereal 3rdparty/hidapi 3rdparty/xxHash 3rdparty/yaml-cpp 3rdparty/libusb 3rdparty/FAudio Vulkan/glslang

# Download pre-compiled llvm libs
curl -sLO https://github.com/RPCS3/llvm/releases/download/continuous-linux-master/llvmlibs-linux.tar.gz
mkdir llvmlibs
tar -xzf ./llvmlibs-linux.tar.gz -C llvmlibs

mkdir build ; cd build

if [ $COMPILER = "gcc" ]; then
	# These are set in the dockerfile
	export CC=${GCC_BINARY}
	export CXX=${GXX_BINARY}
else
	export CC=${CLANG_BINARY}
	export CXX=${CLANGXX_BINARY}
fi

cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_LLVM_SUBMODULE=OFF -DLLVM_DIR=llvmlibs/lib/cmake/llvm/ -DUSE_NATIVE_INSTRUCTIONS=OFF -DUSE_FAUDIO=OFF -G Ninja

ninja; build_status=$?;

cd ..
# If it compiled succesfully let's deploy
if [ $build_status -eq 0 ] && [ -n "$GITHUB_TOKEN" ] && [ "$TRAVIS_BRANCH" = "master" ] && [ "$TRAVIS_PULL_REQUEST" = false ]; then /bin/bash -ex .travis/deploy-linux.bash ; fi
