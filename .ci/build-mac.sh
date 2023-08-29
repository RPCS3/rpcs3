#!/bin/sh -ex

if [ "$(arch -x86_64 echo test)" = "test" ]; then
  echo "Rosetta is installed or building on Intel"
else
  echo "Installing Rosetta"
  /usr/sbin/softwareupdate --install-rosetta --agree-to-license
fi

if [ -z "$INSTALL_DEPS" ]; then
  export INSTALL_DEPS="true"
fi

if [ "$INSTALL_DEPS" = "true" ] ; then
  export HOMEBREW_NO_AUTO_UPDATE=1
  brew -v || /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  brew install -f --overwrite nasm ninja git p7zip create-dmg ccache pipenv

  arch -x86_64 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  arch -x86_64 /usr/local/bin/brew update
  arch -x86_64 /usr/local/bin/brew install --build-from-source ffmpeg gnutls
  arch -x86_64 /usr/local/bin/brew install -f --overwrite llvm@16 glew cmake sdl2 vulkan-headers coreutils
  arch -x86_64 /usr/local/bin/brew link -f llvm@16

  # moltenvk based on commit for 1.2.5 release
  wget https://raw.githubusercontent.com/Homebrew/homebrew-core/b0bba05b617ef0fd796b3727be46addfd098a491/Formula/m/molten-vk.rb
  arch -x86_64 /usr/local/bin/brew install -f --overwrite ./molten-vk.rb
fi

#export MACOSX_DEPLOYMENT_TARGET=12.0
export CXX=clang++
export CC=clang

export BREW_PATH;
BREW_PATH="$(brew --prefix)"
export BREW_X64_PATH;
BREW_X64_PATH="$("/usr/local/bin/brew" --prefix)"
export BREW_BIN="/usr/local/bin"
export BREW_SBIN="/usr/local/sbin"
export CMAKE_EXTRA_OPTS='-DLLVM_TARGETS_TO_BUILD=X86'

export WORKDIR;
WORKDIR="$(pwd)"

# Get Qt
if [ -z "$QT_VER" ]; then
  export QT_VER="6.5.2"
fi

if [ ! -d "/tmp/Qt/$QT_VER" ]; then
  mkdir -p "/tmp/Qt"
  rm -rf qt-downloader
  git clone https://github.com/engnr/qt-downloader.git
  cd qt-downloader
  git checkout f52efee0f18668c6d6de2dec0234b8c4bc54c597
  cd "/tmp/Qt"
  "/opt/homebrew/bin/pipenv" run pip3 install py7zr requests semantic_version lxml
  mkdir -p "$QT_VER/macos" ; ln -s "macos" "$QT_VER/clang_64"
  "/opt/homebrew/bin/pipenv" run "$WORKDIR/qt-downloader/qt-downloader" macos desktop "$QT_VER" clang_64 --opensource --addons qtmultimedia
fi

cd "$WORKDIR"
ditto "/tmp/Qt/$QT_VER" "qt-downloader/$QT_VER"

export Qt6_DIR="$WORKDIR/qt-downloader/$QT_VER/clang_64/lib/cmake/Qt$QT_VER_MAIN"
export SDL2_DIR="$BREW_X64_PATH/opt/sdl2/lib/cmake/SDL2"

export PATH="$WORKDIR/qt-downloader/$QT_VER/clang_64/bin:$BREW_BIN:$BREW_SBIN:/usr/bin:/bin:/usr/sbin:/sbin:/opt/X11/bin:/Library/Apple/usr/bin:$PATH"

if [ "$USE_APPLE_CLANG" = "true" ]; then
  echo "Building using Apple Clang"
else
  echo "Building using Homebrew Clang"
  export PATH="$BREW_X64_PATH/opt/llvm@16/bin:$PATH"
fi

export LDFLAGS="-L$BREW_X64_PATH/lib -Wl,-rpath,$BREW_X64_PATH/lib"
export CPPFLAGS="-I$BREW_X64_PATH/include -msse -msse2 -mcx16 -no-pie"
export LIBRARY_PATH="$BREW_X64_PATH/lib"
export LD_LIBRARY_PATH="$BREW_X64_PATH/lib"

export VULKAN_SDK
VULKAN_SDK="$BREW_X64_PATH/opt/molten-vk"
ln -s "$VULKAN_SDK/lib/libMoltenVK.dylib" "$VULKAN_SDK/lib/libvulkan.dylib" || echo "Using existing libvulkan.dylib"
export VK_ICD_FILENAMES="$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"

export LLVM_DIR
LLVM_DIR="BREW_X64_PATH/opt/llvm@16"
# exclude ffmpeg, SPIRV and LLVM, and sdl from submodule update
# shellcheck disable=SC2046
git submodule -q update --init --depth=1 --jobs=8 $(awk '/path/ && !/ffmpeg/ && !/llvm/ && !/SPIRV/ && !/SDL/ { print $3 }' .gitmodules)

# 3rdparty fixes
sed -i '' "s/extern const double NSAppKitVersionNumber;/const double NSAppKitVersionNumber = 1343;/g" 3rdparty/hidapi/hidapi/mac/hid.c

if [ "$CLEAN_BUILDDIR" = "true" ]; then
  rm -rf build || echo "build directory not deleted"
fi

mkdir -p build && cd build || exit 1

"$BREW_X64_PATH/bin/cmake" .. \
    -DUSE_SDL=ON \
    -DUSE_DISCORD_RPC=ON \
    -DUSE_VULKAN=ON \
    -DUSE_ALSA=OFF \
    -DUSE_PULSE=OFF \
    -DUSE_AUDIOUNIT=ON \
    -DUSE_SYSTEM_FFMPEG=ON \
    -DLLVM_CCACHE_BUILD=OFF \
    -DLLVM_BUILD_RUNTIME=OFF \
    -DLLVM_BUILD_TOOLS=OFF \
    -DLLVM_INCLUDE_DOCS=OFF \
    -DLLVM_INCLUDE_EXAMPLES=OFF \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_INCLUDE_TOOLS=OFF \
    -DLLVM_INCLUDE_UTILS=OFF \
    -DLLVM_USE_PERF=OFF \
    -DLLVM_ENABLE_Z3_SOLVER=OFF \
    -DUSE_NATIVE_INSTRUCTIONS=OFF \
    -DUSE_SYSTEM_MVK=ON \
    -DUSE_SYSTEM_FAUDIO=OFF \
    -DUSE_SYSTEM_SDL=ON \
    $CMAKE_EXTRA_OPTS \
    -DLLVM_TARGET_ARCH=X86_64 \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 \
    -DCMAKE_IGNORE_PATH="$BREW_PATH/lib" \
    -G Ninja

"$BREW_PATH/bin/ninja"; build_status=$?;

cd ..

{   [ "$CI_HAS_ARTIFACTS" = "true" ];
} && SHOULD_DEPLOY="true" || SHOULD_DEPLOY="false"

if [ "$build_status" -eq 0 ] && [ "$SHOULD_DEPLOY" = "true" ]; then
    .ci/deploy-mac.sh
fi
