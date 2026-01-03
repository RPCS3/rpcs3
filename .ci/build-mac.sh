#!/bin/sh -ex

export HOMEBREW_NO_AUTO_UPDATE=1
export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
export HOMEBREW_NO_ENV_HINTS=1
export HOMEBREW_NO_INSTALL_CLEANUP=1
brew install -f --overwrite --quiet ccache "llvm@$LLVM_COMPILER_VER"
brew link -f --overwrite --quiet "llvm@$LLVM_COMPILER_VER"
if [ "$AARCH64" -eq 1 ]; then
  brew install -f --overwrite --quiet googletest opencv@4 sdl3 vulkan-headers vulkan-loader molten-vk 
  brew unlink --quiet ffmpeg fmt qtbase qtsvg qtdeclarative
else
  arch -x86_64 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  arch -x86_64 /usr/local/bin/brew install -f --overwrite --quiet python@3.14 opencv@4 "llvm@$LLVM_COMPILER_VER" sdl3 vulkan-headers vulkan-loader molten-vk
  arch -x86_64 /usr/local/bin/brew unlink  --quiet ffmpeg qtbase qtsvg qtdeclarative
  arch -x86_64 /usr/local/bin/brew link -f --overwrite --quiet "llvm@$LLVM_COMPILER_VER"
fi

export CXX=clang++
export CC=clang

export BREW_PATH;
if [ "$AARCH64" -eq 1 ]; then
  BREW_PATH="$(brew --prefix)"
  export BREW_BIN="/opt/homebrew/bin"
  export BREW_SBIN="/opt/homebrew/sbin"
else
  BREW_PATH="$("/usr/local/bin/brew" --prefix)"
  export BREW_BIN="/usr/local/bin"
  export BREW_SBIN="/usr/local/sbin"
fi

export WORKDIR;
WORKDIR="$(pwd)"

# Setup ccache
if [ ! -d "$CCACHE_DIR" ]; then
  mkdir -p "$CCACHE_DIR"
fi

# Get Qt
if [ ! -d "/tmp/Qt/$QT_VER" ]; then
  mkdir -p "/tmp/Qt"
  git clone https://github.com/engnr/qt-downloader.git
  cd qt-downloader
  git checkout f52efee0f18668c6d6de2dec0234b8c4bc54c597
  sed -i '' "s/'qt{0}_{0}{1}{2}'.format(major, minor, patch)]))/'qt{0}_{0}{1}{2}'.format(major, minor, patch), 'qt{0}_{0}{1}{2}'.format(major, minor, patch)]))/g" qt-downloader
  sed -i '' "s/'{}\/{}\/qt{}_{}\/'/'{0}\/{1}\/qt{2}_{3}\/qt{2}_{3}\/'/g" qt-downloader
  cd "/tmp/Qt"
  pip3 install py7zr requests semantic_version lxml --no-cache --break-system-packages
  mkdir -p "$QT_VER/macos" ; ln -s "macos" "$QT_VER/clang_64"
  sed -i '' 's/args\.version \/ derive_toolchain_dir(args) \/ //g' "$WORKDIR/qt-downloader/qt-downloader"
  python3 "$WORKDIR/qt-downloader/qt-downloader" macos desktop "$QT_VER" clang_64 --opensource --addons qtmultimedia qtimageformats -o "$QT_VER/clang_64"
fi

cd "$WORKDIR"
ditto "/tmp/Qt/$QT_VER" "qt-downloader/$QT_VER"

export Qt6_DIR="$WORKDIR/qt-downloader/$QT_VER/clang_64/lib/cmake/Qt$QT_VER_MAIN"
export SDL3_DIR="$BREW_PATH/opt/sdl3/lib/cmake/SDL3"

export PATH="/opt/homebrew/opt/llvm@$LLVM_COMPILER_VER/bin:$PATH"
export LDFLAGS="-L$BREW_PATH/opt/llvm@$LLVM_COMPILER_VER/lib/c++ -L$BREW_PATH/opt/llvm@$LLVM_COMPILER_VER/lib/unwind -lunwind"

export VULKAN_SDK
VULKAN_SDK="$BREW_PATH/opt/molten-vk"
ln -s "$BREW_PATH/opt/vulkan-loader/lib/libvulkan.dylib" "$VULKAN_SDK/lib/libvulkan.dylib"

export LLVM_DIR
LLVM_DIR="$BREW_PATH/opt/llvm@$LLVM_COMPILER_VER"
# Pull all the submodules except some
# shellcheck disable=SC2046
git submodule -q update --init --depth=1 --jobs=8 $(awk '/path/ && !/llvm/ && !/opencv/ && !/SDL/ && !/feralinteractive/ { print $3 }' .gitmodules)

mkdir build && cd build || exit 1

if [ "$AARCH64" -eq 1 ]; then
cmake .. \
    -DBUILD_RPCS3_TESTS="${RUN_UNIT_TESTS}" \
    -DRUN_RPCS3_TESTS="${RUN_UNIT_TESTS}" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.4 \
    -DCMAKE_OSX_SYSROOT="$(xcrun --sdk macosx --show-sdk-path)" \
    -DSTATIC_LINK_LLVM=ON \
    -DUSE_SDL=ON \
    -DUSE_DISCORD_RPC=ON \
    -DUSE_AUDIOUNIT=ON \
    -DUSE_SYSTEM_FFMPEG=OFF \
    -DUSE_NATIVE_INSTRUCTIONS=OFF \
    -DUSE_PRECOMPILED_HEADERS=OFF \
    -DUSE_SYSTEM_MVK=ON \
    -DUSE_SYSTEM_SDL=ON \
    -DUSE_SYSTEM_OPENCV=ON \
    -G Ninja
else
cmake .. \
    -DBUILD_RPCS3_TESTS=OFF \
    -DRUN_RPCS3_TESTS=OFF \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 \
    -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
    -DCMAKE_TOOLCHAIN_FILE=buildfiles/cmake/TCDarwinX86_64.cmake \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.4 \
    -DCMAKE_OSX_SYSROOT="$(xcrun --sdk macosx --show-sdk-path)" \
    -DSTATIC_LINK_LLVM=ON \
    -DUSE_SDL=ON \
    -DUSE_DISCORD_RPC=ON \
    -DUSE_AUDIOUNIT=ON \
    -DUSE_SYSTEM_FFMPEG=OFF \
    -DUSE_NATIVE_INSTRUCTIONS=OFF \
    -DUSE_PRECOMPILED_HEADERS=OFF \
    -DUSE_SYSTEM_MVK=ON \
    -DUSE_SYSTEM_SDL=ON \
    -DUSE_SYSTEM_OPENCV=ON \
    -G Ninja
fi

ninja; build_status=$?;

cd ..

# If it compiled succesfully let's deploy.
if [ "$build_status" -eq 0 ]; then
    .ci/deploy-mac.sh
fi
