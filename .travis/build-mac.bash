brew install ccache glew ninja qt
export CCACHE_SLOPPINESS=pch_defines,time_macros
export CMAKE_PREFIX_PATH=/usr/local/opt/qt5/
export PATH="/usr/local/opt/ccache/libexec:$PATH"

# Allow using optional on 10.13
sudo perl -pi -e 'BEGIN{undef $/;} s/(_LIBCPP_AVAILABILITY_BAD_OPTIONAL_ACCESS[ ]+\\\n[^\n]+10)\.14/\1.13/;' \
  $(xcode-select -p)/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1/__config

# Setup vulkan and gfx-rs/portability
curl -sLO https://github.com/gfx-rs/portability/releases/download/latest/gfx-portability-macos-latest.zip
unzip -: gfx-portability-macos-latest.zip
curl -sLO https://github.com/KhronosGroup/Vulkan-Headers/archive/sdk-1.1.85.0.zip
unzip -: sdk-*.zip
mkdir vulkan-sdk
ln -s ${PWD}/Vulkan-Headers*/include vulkan-sdk/include
mkdir vulkan-sdk/lib
ln target/release/libportability.dylib vulkan-sdk/lib/libVulkan.dylib
export VULKAN_SDK=${PWD}/vulkan-sdk

git submodule update --quiet --init asmjit 3rdparty/ffmpeg 3rdparty/pugixml 3rdparty/GSL 3rdparty/libpng 3rdparty/cereal 3rdparty/hidapi 3rdparty/xxHash 3rdparty/yaml-cpp Vulkan/glslang

mkdir build; cd build
cmake .. -DWITHOUT_LLVM=On -DUSE_NATIVE_INSTRUCTIONS=OFF -G Ninja
ninja
