#!/bin/sh -ex

export CCACHE_SLOPPINESS=pch_defines,time_macros
export CMAKE_PREFIX_PATH=/usr/local/opt/qt5/
export PATH="/usr/local/opt/ccache/libexec:$PATH"

# Setup vulkan and gfx-rs/portability
curl -sLO https://github.com/gfx-rs/portability/releases/download/latest/gfx-portability-macos-latest.zip
unzip -: gfx-portability-macos-latest.zip
curl -sLO https://github.com/KhronosGroup/Vulkan-Headers/archive/sdk-1.1.106.0.zip
unzip -: sdk-*.zip
mkdir vulkan-sdk
ln -s "${PWD}"/Vulkan-Headers*/include vulkan-sdk/include
mkdir vulkan-sdk/lib
cp target/release/libportability.dylib vulkan-sdk/lib/libVulkan.dylib
# Let macdeployqt locate and install Vulkan library
install_name_tool -id "${PWD}"/vulkan-sdk/lib/libVulkan.dylib vulkan-sdk/lib/libVulkan.dylib
export VULKAN_SDK="${PWD}/vulkan-sdk"

# Pull all the submodules except llvm
# Note: Tried to use git submodule status, but it takes over 20 seconds
# shellcheck disable=SC2046
git submodule -q update --init $(awk '/path/ && !/llvm/ { print $3 }' .gitmodules)

mkdir build && cd build || exit 1
cmake .. -DWITH_LLVM=OFF -DUSE_NATIVE_INSTRUCTIONS=OFF -G Ninja
ninja
