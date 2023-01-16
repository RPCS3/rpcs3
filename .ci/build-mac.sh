#!/bin/sh -ex

brew install -f --overwrite llvm@14 nasm ninja git p7zip create-dmg ccache

#/usr/sbin/softwareupdate --install-rosetta --agree-to-license
arch -x86_64 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
arch -x86_64 /usr/local/homebrew/bin/brew install -f --overwrite llvm@14 sdl2 qt@5 glew cmake

#export MACOSX_DEPLOYMENT_TARGET=12.0
export CXX=clang++
export CC=clang

export BREW_PATH;
BREW_PATH="$(brew --prefix)"
export BREW_X64_PATH;
BREW_X64_PATH="$("/usr/local/homebrew/bin/brew" --prefix)"
export BREW_BIN="/usr/local/bin"
export BREW_SBIN="/usr/local/sbin"
export CMAKE_EXTRA_OPTS='-DLLVM_TARGETS_TO_BUILD=X86'

export Qt5_DIR="$BREW_X64_PATH/opt/qt@5/lib/cmake/Qt5"
export SDL2_DIR="$BREW_X64_PATH/opt/qt@5/lib/cmake/SDL2"

export PATH="$BREW_PATH/opt/llvm@14/bin:$BREW_BIN:$BREW_SBIN:/usr/bin:/bin:/usr/sbin:/sbin:/opt/X11/bin:/Library/Apple/usr/bin:$PATH"
export LDFLAGS="-L$BREW_X64_PATH/lib -Wl,-rpath,$BREW_X64_PATH/lib"
export CPPFLAGS="-I$BREW_X64_PATH/include -msse -msse2 -mcx16 -no-pie"
export LIBRARY_PATH="$BREW_X64_PATH/lib"
export LD_LIBRARY_PATH="$BREW_X64_PATH/lib"

git submodule update --init --recursive --depth 1

# 3rdparty fixes
sed -i '' "s/extern const double NSAppKitVersionNumber;/const double NSAppKitVersionNumber = 1343;/g" 3rdparty/hidapi/hidapi/mac/hid.c

mkdir build && cd build || exit 1

"$BREW_X64_PATH/bin/cmake" .. \
    -DUSE_DISCORD_RPC=OFF -DUSE_VULKAN=ON -DUSE_ALSA=OFF -DUSE_PULSE=OFF -DUSE_AUDIOUNIT=ON \
    -DLLVM_CCACHE_BUILD=OFF -DLLVM_BUILD_RUNTIME=OFF -DLLVM_BUILD_TOOLS=OFF \
    -DLLVM_INCLUDE_DOCS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_TOOLS=OFF \
    -DLLVM_INCLUDE_UTILS=OFF -DLLVM_USE_PERF=OFF -DLLVM_ENABLE_Z3_SOLVER=OFF \
    -DUSE_NATIVE_INSTRUCTIONS=OFF \
    -DUSE_SYSTEM_MVK=OFF \
    $CMAKE_EXTRA_OPTS \
    -DLLVM_TARGET_ARCH=X86_64 -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_IGNORE_PATH="$BREW_PATH/lib" \
    -G Ninja

"$BREW_PATH/bin/ninja"; build_status=$?;

cd ..

{   [ "$CI_HAS_ARTIFACTS" = "true" ];
} && SHOULD_DEPLOY="true" || SHOULD_DEPLOY="false"

if [ "$build_status" -eq 0 ] && [ "$SHOULD_DEPLOY" = "true" ]; then
    .ci/deploy-mac.sh
fi
