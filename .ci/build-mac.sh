#!/bin/sh -ex

brew update
brew install llvm@13 sdl2 nasm qt@5 ninja cmake glew git p7zip create-dmg ccache

export MACOSX_DEPLOYMENT_TARGET=11.6
export CXX=clang++
export CC=clang
export Qt5_DIR="/usr/local/opt/qt@5/lib/cmake/Qt5"
export PATH="/usr/local/opt/llvm/bin:/usr/local/bin:/usr/local/sbin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:/opt/X11/bin:/Library/Apple/usr/bin"
export LDFLAGS="-L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib"
export CPPFLAGS="-I/usr/local/opt/llvm/include -msse -msse2 -mcx16 -no-pie"
export VULKAN_SDK="$CIRRUS_WORKING_DIR/3rdparty/MoltenVK/MoltenVK/MoltenVK"
export VK_ICD_FILENAMES="$VULKAN_SDK/dylib/macOS/MoltenVK_icd.json"
export CPLUS_INCLUDE_PATH="$VULKAN_SDK/include"
export Vulkan_INCLUDE_DIR="$VULKAN_SDK/include"
export Vulkan_LIBRARY="$VULKAN_SDK/lib/libvulkan.dylib"

git submodule update --init --recursive --depth 1

# 3rdparty fixes
sed -i '' "s/extern const double NSAppKitVersionNumber;/const double NSAppKitVersionNumber = 1343;/g" 3rdparty/hidapi/hidapi/mac/hid.c

# build MoltenVK
cd 3rdparty/MoltenVK/MoltenVK
./fetchDependencies --macos
xcodebuild build -quiet -project MoltenVKPackaging.xcodeproj -scheme "MoltenVK Package (macOS only)" -configuration "Release" -arch "x86_64"
cd ../../../
mkdir "$VULKAN_SDK/lib"
ln "$VULKAN_SDK/dylib/macOS/libMoltenVK.dylib" "$VULKAN_SDK/lib/libMoltenVK.dylib"
ln "$VULKAN_SDK/dylib/macOS/libMoltenVK.dylib" "$VULKAN_SDK/lib/libvulkan.dylib"
ln "$VULKAN_SDK/MoltenVK.xcframework/macos-x86_64/libMoltenVK.a" "$VULKAN_SDK/lib/libvulkan.a"
mkdir "$VULKAN_SDK/bin"
ln -s "$VULKAN_SDK/../Package/Release/MoltenVKShaderConverter/Tools/MoltenVKShaderConverter" "$VULKAN_SDK/bin/MoltenVKShaderConverter"
ln -s "$VULKAN_SDK/../Package/Release/MoltenVKShaderConverter/include/MoltenVKShaderConverter" "$VULKAN_SDK/include/MoltenVKShaderConverter"
mkdir -p "$VULKAN_SDK/share/vulkan/icd.d"
ln "$VULKAN_SDK/icd/MoltenVK_icd.json" "$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"

mkdir build && cd build || exit 1

cmake .. \
    -DUSE_DISCORD_RPC=OFF -DUSE_VULKAN=ON -DUSE_ALSA=OFF -DUSE_PULSE=OFF -DUSE_AUDIOUNIT=ON \
    -DLLVM_CCACHE_BUILD=OFF -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_BUILD_RUNTIME=OFF -DLLVM_BUILD_TOOLS=OFF \
    -DLLVM_INCLUDE_DOCS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_TOOLS=OFF \
    -DLLVM_INCLUDE_UTILS=OFF -DLLVM_USE_PERF=OFF -DLLVM_ENABLE_Z3_SOLVER=OFF \
    -DUSE_NATIVE_INSTRUCTIONS=OFF \
    -G Ninja

ninja; build_status=$?;

cd ..

{   [ "$CI_HAS_ARTIFACTS" = "true" ];
} && SHOULD_DEPLOY="true" || SHOULD_DEPLOY="false"

if [ "$build_status" -eq 0 ] && [ "$SHOULD_DEPLOY" = "true" ]; then
    .ci/deploy-mac.sh
fi