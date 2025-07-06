#!/bin/sh -ex

# shellcheck disable=SC2086
export HOMEBREW_NO_AUTO_UPDATE=1
export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
export HOMEBREW_NO_ENV_HINTS=1
export HOMEBREW_NO_INSTALL_CLEANUP=1

/opt/homebrew/bin/brew install -f --overwrite --quiet nasm ninja p7zip ccache pipenv gnutls freetype googletest #create-dmg
/opt/homebrew/bin/brew install -f --quiet ffmpeg@5
/opt/homebrew/bin/brew install --quiet "llvm@$LLVM_COMPILER_VER" glew cmake sdl3 vulkan-headers coreutils
/opt/homebrew/bin/brew link -f --quiet "llvm@$LLVM_COMPILER_VER" ffmpeg@5

# moltenvk based on commit for 1.3.0 release
wget https://raw.githubusercontent.com/Homebrew/homebrew-core/7255441cbcafabaa8950f67c7ec55ff499dbb2d3/Formula/m/molten-vk.rb
/opt/homebrew/bin/brew install -f --overwrite --formula --quiet ./molten-vk.rb
export CXX=clang++
export CC=clang

export BREW_PATH;
BREW_PATH="$(brew --prefix)"
export BREW_BIN="/opt/homebrew/bin"
export BREW_SBIN="/opt/homebrew/sbin"
export CMAKE_EXTRA_OPTS='-DLLVM_TARGETS_TO_BUILD=arm64'

export WORKDIR;
WORKDIR="$(pwd)"

# Get Qt
if [ ! -d "/tmp/Qt/$QT_VER" ]; then
  mkdir -p "/tmp/Qt"
  git clone https://github.com/engnr/qt-downloader.git
  cd qt-downloader
  git checkout f52efee0f18668c6d6de2dec0234b8c4bc54c597
  # nested Qt 6.9.1 URL workaround
  # sed -i '' "s/'qt{0}_{0}{1}{2}'.format(major, minor, patch)]))/'qt{0}_{0}{1}{2}'.format(major, minor, patch), 'qt{0}_{0}{1}{2}'.format(major, minor, patch)]))/g" qt-downloader
  # sed -i '' "s/'{}\/{}\/qt{}_{}\/'/'{0}\/{1}\/qt{2}_{3}\/qt{2}_{3}\/'/g" qt-downloader
  # archived Qt 6.7.3 URL workaround
  sed -i '' "s/official_releases/archive/g" qt-downloader
  cd "/tmp/Qt"
  arch -arm64 "$BREW_PATH/bin/pipenv" run pip3 uninstall py7zr requests semantic_version lxml
  arch -arm64 "$BREW_PATH/bin/pipenv" run pip3 install py7zr requests semantic_version lxml  --no-cache
  mkdir -p "$QT_VER/macos" ; ln -s "macos" "$QT_VER/clang_64"
  # sed -i '' 's/args\.version \/ derive_toolchain_dir(args) \/ //g' "$WORKDIR/qt-downloader/qt-downloader" # Qt 6.9.1 workaround
  arch -arm64 "$BREW_PATH/bin/pipenv" run "$WORKDIR/qt-downloader/qt-downloader" macos desktop "$QT_VER" clang_64 --opensource --addons qtmultimedia qtimageformats # -o "$QT_VER/clang_64"
fi

cd "$WORKDIR"
ditto "/tmp/Qt/$QT_VER" "qt-downloader/$QT_VER"

export Qt6_DIR="$WORKDIR/qt-downloader/$QT_VER/clang_64/lib/cmake/Qt$QT_VER_MAIN"
export SDL3_DIR="$BREW_PATH/opt/sdl3/lib/cmake/SDL3"

export PATH="$BREW_PATH/opt/llvm@$LLVM_COMPILER_VER/bin:$WORKDIR/qt-downloader/$QT_VER/clang_64/bin:$BREW_BIN:$BREW_SBIN:/usr/bin:/bin:/usr/sbin:/sbin:/opt/X11/bin:/Library/Apple/usr/bin:$PATH"
export LDFLAGS="-L$BREW_PATH/lib $BREW_PATH/opt/ffmpeg@5/lib/libavcodec.dylib $BREW_PATH/opt/ffmpeg@5/lib/libavformat.dylib $BREW_PATH/opt/ffmpeg@5/lib/libavutil.dylib $BREW_PATH/opt/ffmpeg@5/lib/libswscale.dylib $BREW_PATH/opt/ffmpeg@5/lib/libswresample.dylib $BREW_PATH/opt/llvm@$LLVM_COMPILER_VER/lib/c++/libc++.1.dylib $BREW_PATH/lib/libSDL3.dylib $BREW_PATH/lib/libGLEW.dylib $BREW_PATH/opt/llvm@$LLVM_COMPILER_VER/lib/unwind/libunwind.1.dylib -Wl,-rpath,$BREW_PATH/lib"
export CPPFLAGS="-I$BREW_PATH/include -I$BREW_PATH/include -no-pie -D__MAC_OS_X_VERSION_MIN_REQUIRED=140000"
export CFLAGS="-D__MAC_OS_X_VERSION_MIN_REQUIRED=140000"
export LIBRARY_PATH="$BREW_PATH/lib"
export LD_LIBRARY_PATH="$BREW_PATH/lib"

export VULKAN_SDK
VULKAN_SDK="$BREW_PATH/opt/molten-vk"
ln -s "$VULKAN_SDK/lib/libMoltenVK.dylib" "$VULKAN_SDK/lib/libvulkan.dylib" || true
export VK_ICD_FILENAMES="$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"

export LLVM_DIR
LLVM_DIR="$BREW_PATH/opt/llvm@$LLVM_COMPILER_VER"
# exclude ffmpeg, LLVM, opencv, and sdl from submodule update
# shellcheck disable=SC2046
git submodule -q update --init --depth=1 --jobs=8 $(awk '/path/ && !/ffmpeg/ && !/llvm/ && !/opencv/ && !/SDL/ && !/feralinteractive/ { print $3 }' .gitmodules)

# 3rdparty fixes
sed -i '' "s/extern const double NSAppKitVersionNumber;/const double NSAppKitVersionNumber = 1343;/g" 3rdparty/hidapi/hidapi/mac/hid.c

mkdir build && cd build || exit 1

export MACOSX_DEPLOYMENT_TARGET=14.0

"$BREW_PATH/bin/cmake" .. \
    -DBUILD_RPCS3_TESTS="${RUN_UNIT_TESTS}" \
    -DRUN_RPCS3_TESTS="${RUN_UNIT_TESTS}" \
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
    -DUSE_SYSTEM_OPENCV=ON \
    "$CMAKE_EXTRA_OPTS" \
    -DLLVM_TARGET_ARCH=arm64 \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_IGNORE_PATH="$BREW_PATH/lib" \
    -DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew/opt \
    -DCMAKE_CXX_FLAGS="-D__MAC_OS_X_VERSION_MIN_REQUIRED=140000" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_OSX_SYSROOT="$(xcrun --sdk macosx --show-sdk-path)" \
    -G Ninja

"$BREW_PATH/bin/ninja"; build_status=$?;

cd ..

# If it compiled succesfully let's deploy.
if [ "$build_status" -eq 0 ]; then
    .ci/deploy-mac-arm64.sh
fi
