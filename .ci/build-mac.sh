#!/bin/sh -ex

brew update
brew install llvm@13 sdl2 nasm qt@5 ninja cmake glew git p7zip create-dmg ccache ffmpeg

export MACOSX_DEPLOYMENT_TARGET=11.6
export CXX=clang++
export CC=clang

ARCH_NAME="$(uname -m)"
if [ "${ARCH_NAME}" = "arm64" ]; then
  export BREW_PATH=/opt/homebrew/opt
  export BREW_BIN=/opt/homebrew/bin
  export BREW_SBIN=/opt/homebrew/sbin
  export CMAKE_EXTRA_OPTS='-DPNG_ARM_NEON=on -DLLVM_TARGETS_TO_BUILD=X86;AArch64;ARM -DCMAKE_OSX_ARCHITECTURES=arm64 -DUSE_SYSTEM_FFMPEG=ON'
else
  export BREW_PATH=/usr/local/opt
  export BREW_BIN=/usr/local/bin
  export BREW_SBIN=/usr/local/sbin
  export CMAKE_EXTRA_OPTS='-DLLVM_TARGETS_TO_BUILD=X86'
fi

export Qt5_DIR="$BREW_PATH/qt@5/lib/cmake/Qt5"
export PATH="$BREW_PATH/llvm@13/bin:$BREW_BIN:$BREW_SBIN:BREW_BIN:/usr/bin:/bin:/usr/sbin:/sbin:/opt/X11/bin:/Library/Apple/usr/bin"
export LDFLAGS="-L$BREW_PATH/llvm@13/lib -Wl,-rpath,$BREW_PATH/llvm@13/lib"
export CPPFLAGS="-I$BREW_PATH/llvm@13/include -msse -msse2 -mcx16 -no-pie"

git submodule update --init --recursive --depth 1

# 3rdparty fixes
sed -i '' "s/extern const double NSAppKitVersionNumber;/const double NSAppKitVersionNumber = 1343;/g" 3rdparty/hidapi/hidapi/mac/hid.c

mkdir build && cd build || exit 1

cmake .. \
    -DUSE_DISCORD_RPC=OFF -DUSE_VULKAN=ON -DUSE_ALSA=OFF -DUSE_PULSE=OFF -DUSE_AUDIOUNIT=ON \
    -DLLVM_CCACHE_BUILD=OFF -DLLVM_BUILD_RUNTIME=OFF -DLLVM_BUILD_TOOLS=OFF \
    -DLLVM_INCLUDE_DOCS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_TOOLS=OFF \
    -DLLVM_INCLUDE_UTILS=OFF -DLLVM_USE_PERF=OFF -DLLVM_ENABLE_Z3_SOLVER=OFF \
    -DUSE_NATIVE_INSTRUCTIONS=OFF \
    -DUSE_SYSTEM_MVK=OFF \
    $CMAKE_EXTRA_OPTS \
    -G Ninja

ninja; build_status=$?;

cd ..

{   [ "$CI_HAS_ARTIFACTS" = "true" ];
} && SHOULD_DEPLOY="true" || SHOULD_DEPLOY="false"

if [ "$build_status" -eq 0 ] && [ "$SHOULD_DEPLOY" = "true" ]; then
    .ci/deploy-mac.sh
fi
