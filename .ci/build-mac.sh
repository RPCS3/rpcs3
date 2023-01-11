#!/bin/sh -ex

export NONINTERACTIVE=1

/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/uninstall.sh)"

arch -x86_64 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
arch -x86_64 /usr/local/homebrew/bin/brew install llvm@14 sdl2 glew cmake nasm ninja p7zip create-dmg ccache
arch -x86_64 /usr/local/homebrew/bin/brew install --build-from-source qt@5

export MACOSX_DEPLOYMENT_TARGET=12.0
export CXX=clang++
export CC=clang
export Qt5_DIR="/usr/local/opt/qt@5/lib/cmake/Qt5"
export PATH="/usr/local/opt/llvm@14/bin:/usr/local/bin:/usr/local/sbin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:/opt/X11/bin:/Library/Apple/usr/bin"
export LDFLAGS="-L/usr/local/opt/llvm@14/lib -Wl,-rpath,/usr/local/opt/llvm@14/lib"
export CPPFLAGS="-I/usr/local/opt/llvm@14/include -msse -msse2 -mcx16 -no-pie"

brew unlink llvm@14
brew link llvm@14

brew unlink sdl2
brew link sdl2

brew unlink qt@5
brew link qt@5

brew unlink glew
brew link glew

brew unlink cmake
brew link cmake

brew unlink nasm
brew link nasm

brew unlink ninja
brew link ninja

brew unlink p7zip
brew link p7zip

brew unlink create-dmg
brew link create-dmg

brew unlink ccache
brew link ccache

git submodule update --init --recursive --depth 1

# 3rdparty fixes
sed -i '' "s/extern const double NSAppKitVersionNumber;/const double NSAppKitVersionNumber = 1343;/g" 3rdparty/hidapi/hidapi/mac/hid.c

mkdir build && cd build || exit 1

cmake .. \
    -DUSE_SDL=ON -DUSE_DISCORD_RPC=OFF -DUSE_VULKAN=ON -DUSE_ALSA=OFF -DUSE_PULSE=OFF -DUSE_AUDIOUNIT=ON \
    -DLLVM_CCACHE_BUILD=OFF -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_BUILD_RUNTIME=OFF -DLLVM_BUILD_TOOLS=OFF \
    -DLLVM_INCLUDE_DOCS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_TOOLS=OFF \
    -DLLVM_INCLUDE_UTILS=OFF -DLLVM_USE_PERF=OFF -DLLVM_ENABLE_Z3_SOLVER=OFF \
    -DUSE_NATIVE_INSTRUCTIONS=OFF \
    -DUSE_SYSTEM_MVK=OFF -DWITH_LLVM=OFF \
    -G Ninja

ninja; build_status=$?;

cd ..

{   [ "$CI_HAS_ARTIFACTS" = "true" ];
} && SHOULD_DEPLOY="true" || SHOULD_DEPLOY="false"

if [ "$build_status" -eq 0 ] && [ "$SHOULD_DEPLOY" = "true" ]; then
    .ci/deploy-mac.sh
fi
