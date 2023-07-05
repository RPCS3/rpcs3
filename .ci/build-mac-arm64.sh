#!/bin/sh -ex

brew update
brew install -f --overwrite nasm ninja git p7zip create-dmg ccache pipenv llvm@16 sdl2 glew cmake faudio qt@5 ffmpeg@6 vulkan-headers
# moltenvk based on commit for 1.2.4 release
wget https://raw.githubusercontent.com/Homebrew/homebrew-core/b233d4f9f40f26d81da11140defbfd578cfe4a69/Formula/molten-vk.rb
brew install -f --overwrite ./molten-vk.rb
brew link -f llvm@16


#export MACOSX_DEPLOYMENT_TARGET=12.0
export CXX=clang++
export CC=clang

export BREW_PATH;
BREW_PATH="$(brew --prefix)"
export BREW_BIN="$BREW_PATH/bin"
export BREW_SBIN="$BREW_PATH/sbin"
export CMAKE_EXTRA_OPTS='-DLLVM_TARGETS_TO_BUILD=AArch64;ARM'

export PATH
PATH="$(brew --prefix llvm@16)/bin:$BREW_BIN:$BREW_SBIN:/usr/bin:/bin:/usr/sbin:/sbin:/opt/X11/bin:/Library/Apple/usr/bin:$PATH"
export LDFLAGS="-L$BREW_PATH/lib -Wl,-rpath,$BREW_PATH/lib"
export CPPFLAGS="-I$BREW_PATH/include"
export LIBRARY_PATH="$BREW_PATH/lib"
export LD_LIBRARY_PATH="$BREW_PATH/lib"
export Qt5_DIR
Qt5_DIR="$(brew --prefix qt@5)/lib/cmake/Qt5"

export VULKAN_SDK
VULKAN_SDK="$(brew --prefix molten-vk)"
ln -s "$VULKAN_SDK/lib/libMoltenVK.dylib" "$VULKAN_SDK/lib/libvulkan.dylib"
export VK_ICD_FILENAMES="$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"
export LLVM_DIR
LLVM_DIR="$(brew --prefix llvm@16)"

# exclude FAudio, SPIRV and LLVM from submodule update
git submodule -q update --init --depth=1 --jobs=8 "$(awk '/path/ && !/FAudio/ && !/llvm/ && !/SPIRV/ { print $3 }' .gitmodules)"

# 3rdparty fixes
sed -i '' "s/extern const double NSAppKitVersionNumber;/const double NSAppKitVersionNumber = 1343;/g" 3rdparty/hidapi/hidapi/mac/hid.c

mkdir build && cd build || exit 1

"$BREW_PATH/bin/cmake" .. \
    -DUSE_SDL=ON -DUSE_DISCORD_RPC=OFF -DUSE_VULKAN=ON -DUSE_ALSA=OFF -DUSE_PULSE=OFF -DUSE_AUDIOUNIT=ON \
    -DLLVM_CCACHE_BUILD=OFF -DLLVM_BUILD_RUNTIME=OFF -DLLVM_BUILD_TOOLS=OFF \
    -DLLVM_INCLUDE_DOCS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_TOOLS=OFF \
    -DLLVM_INCLUDE_UTILS=OFF -DLLVM_USE_PERF=OFF -DLLVM_ENABLE_Z3_SOLVER=OFF \
    -DUSE_NATIVE_INSTRUCTIONS=OFF \
    -DUSE_SYSTEM_FAUDIO=ON \
    -DUSE_SYSTEM_MVK=OFF \
    -DUSE_SYSTEM_SDL=ON \
    -DUSE_SYSTEM_FFMPEG=OFF \
    -DPNG_ARM_NEON=on \
    $CMAKE_EXTRA_OPTS \
    -DLLVM_TARGET_ARCH="AArch64;ARM" -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -G Ninja

"$BREW_PATH/bin/ninja"; build_status=$?;

cd ..
export BUILDING_FOR="arm64"
{   [ "$CI_HAS_ARTIFACTS" = "true" ];
} && SHOULD_DEPLOY="true" || SHOULD_DEPLOY="false"

if [ "$build_status" -eq 0 ] && [ "$SHOULD_DEPLOY" = "true" ]; then
    .ci/deploy-mac.sh
fi