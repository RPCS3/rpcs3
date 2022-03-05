#!/bin/sh -ex

brew update
brew install llvm@13 molten-vk vulkan-headers sdl2 nasm qt@5 ninja cmake glew git p7zip create-dmg ccache

export CXX=clang++
export CC=clang
export Qt5_DIR="/usr/local/opt/qt@5/lib/cmake/Qt5"
export PATH="/usr/local/opt/llvm/bin:/usr/local/bin:/usr/local/sbin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:/opt/X11/bin:/Library/Apple/usr/bin"
export LDFLAGS="-L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib"
export CPPFLAGS="-I/usr/local/opt/llvm/include -msse -msse2 -mcx16 -no-pie"
export CPLUS_INCLUDE_PATH="/usr/local/opt/molten-vk/include"
export VULKAN_SDK="/usr/local/opt/molten-vk"
export VK_ICD_FILENAMES="$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"

git submodule update --init --recursive --depth 1

# 3rdparty fixes
ln -s "$VULKAN_SDK/lib/libMoltenVK.dylib" "$VULKAN_SDK/lib/libvulkan.dylib"
sed -i '' "s/extern const double NSAppKitVersionNumber;/const double NSAppKitVersionNumber = 1343;/g" 3rdparty/hidapi/hidapi/mac/hid.c

cd ..
mkdir build && cd build || exit 1
cmake -DUSE_DISCORD_RPC=OFF -DUSE_VULKAN=ON -DUSE_ALSA=OFF -DUSE_PULSE=OFF -DUSE_AUDIOUNIT=ON \
-G Ninja -DLLVM_CCACHE_BUILD=OFF -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_BUILD_RUNTIME=OFF -DLLVM_BUILD_TOOLS=OFF \
-DLLVM_INCLUDE_DOCS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_TOOLS=OFF \
-DLLVM_INCLUDE_UTILS=OFF -DLLVM_USE_PERF=OFF -DLLVM_ENABLE_Z3_SOLVER=OFF \
-DCMAKE_CXX_STANDARD=20 -DUSE_NATIVE_INSTRUCTIONS=OFF \
../cirrus-ci-build/
ninja

cd ../cirrus-ci-build/
COMM_TAG="$(grep 'version{.*}' rpcs3/rpcs3_version.cpp | awk -F[\{,] '{printf "%d.%d.%d", $2, $3, $4}')"
COMM_COUNT="$(git rev-list --count HEAD)"
COMM_HASH="$(git rev-parse --short=8 HEAD)"
cd ../build/bin

mkdir "rpcs3.app/Contents/lib/"
cp "/usr/local/opt/llvm/lib/libc++abi.1.0.dylib" "rpcs3.app/Contents/lib/libc++abi.1.dylib"
rm -rf "rpcs3.app/Contents/Frameworks/QtPdf.framework" \
"rpcs3.app/Contents/Frameworks/QtQml.framework" \
"rpcs3.app/Contents/Frameworks/QtQmlModels.framework" \
"rpcs3.app/Contents/Frameworks/QtQuick.framework" \
"rpcs3.app/Contents/Frameworks/QtVirtualKeyboard.framework" \
"rpcs3.app/Contents/Plugins/platforminputcontexts" \
"rpcs3.app/Contents/Plugins/virtualkeyboard"

mv rpcs3.app RPCS3_.app
mv RPCS3_.app RPCS3.app

echo "[InternetShortcut]" > Quickstart.url
echo "URL=https://rpcs3.net/quickstart" >> Quickstart.url
echo "IconIndex=0" >> Quickstart.url

create-dmg --volname RPCS3 \
--window-size 800 400 \
--icon-size 100 \
--icon rpcs3.app 200 190 \
--add-file Quickstart.url Quickstart.url 400 20 \
--hide-extension rpcs3.app \
--hide-extension Quickstart.url \
--app-drop-link 600 185 \
--skip-jenkins \
"$ARTDIR/rpcs3-v${COMM_TAG}-${COMM_COUNT}-${COMM_HASH}_macos.dmg" \
RPCS3.app

7z a -mx9 rpcs3-v"${COMM_TAG}"-"${COMM_COUNT}"-"${COMM_HASH}"_macos.7z RPCS3.app
mv rpcs3-v"${COMM_TAG}"-"${COMM_COUNT}"-"${COMM_HASH}"_macos.7z "$ARTDIR"
