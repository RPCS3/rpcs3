#!/bin/sh -ex

# shellcheck disable=SC2086
brew_arm64_install_packages() {
    for pkg in "$@"; do
        echo "Fetching bottle for $pkg (arm64)..."
        bottle_path="$("$BREW_ARM64_PATH/bin/brew" --cache --bottle-tag=arm64_ventura "$pkg")"
        if [ ! -f "$bottle_path" ]; then
            if ! "$BREW_ARM64_PATH/bin/brew" fetch --force --verbose --debug --bottle-tag=arm64_ventura "$pkg"; then
                echo "Failed to fetch bottle for $pkg"
                return 1
            fi
            bottle_path="$("$BREW_ARM64_PATH/bin/brew" --cache --bottle-tag=arm64_ventura "$pkg")"
        fi

        echo "Installing $pkg (arm64)..."
        "$BREW_ARM64_PATH/bin/brew" install --force --force-bottle --ignore-dependencies "$bottle_path" || true
    done
}

export HOMEBREW_NO_AUTO_UPDATE=1
export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
export HOMEBREW_NO_INSTALL_CLEANUP=1

/usr/local/bin/brew update
sudo rm -rf /usr/local/Cellar/curl /usr/local/opt/curl
/usr/local/bin/brew install -f --overwrite curl
/usr/local/bin/brew uninstall -f --ignore-dependencies ffmpeg
/usr/local/bin/brew install -f --build-from-source ffmpeg@5 || true
/usr/local/bin/brew install -f --overwrite python || true
/usr/local/bin/brew link --overwrite python || true
/usr/local/bin/brew install -f --overwrite nasm ninja p7zip ccache pipenv #create-dmg
/usr/local/bin/brew link -f curl || true
/usr/local/bin/brew install llvm@$LLVM_COMPILER_VER glew cmake sdl2 vulkan-headers coreutils
/usr/local/bin/brew link -f llvm@$LLVM_COMPILER_VER ffmpeg@5 || true

export BREW_ARM64_PATH="/opt/homebrew1"
sudo mkdir -p "$BREW_ARM64_PATH"
sudo chmod 777 "$BREW_ARM64_PATH"
curl -L https://github.com/Homebrew/brew/tarball/master | tar xz --strip 1 -C "$BREW_ARM64_PATH"

#"$BREW_ARM64_PATH/bin/brew" update
# libvorbis requires Homebrew-installed curl, but we can't run it on x64, and we also need the aarch64 libs, so we swap the binary
brew_arm64_install_packages curl
mv /opt/homebrew1/opt/curl/bin/curl /opt/homebrew1/opt/curl/bin/curl.bak
ln -s /usr/local/opt/curl/bin/curl /opt/homebrew1/opt/curl/bin/curl

brew_arm64_install_packages 0mq aom aribb24 ca-certificates cjson dav1d ffmpeg@5 fontconfig freetype freetype2 gettext glew gmp gnutls lame libbluray libidn2 libnettle libogg libpng librist libsodium libsoxr libtasn libtasn1 libunistring libvmaf libvorbis libvpx libx11 libxau libxcb libxdmcp llvm@$LLVM_COMPILER_VER mbedtls molten-vk nettle opencore-amr openjpeg openssl opus p11-kit pkg-config pkgconfig pzstd rav1e sdl2 snappy speex srt svt-av1 theora vulkan-headers webp x264 x265 xz z3 zeromq zmq zstd
"$BREW_ARM64_PATH/bin/brew" link -f ffmpeg@5

# moltenvk based on commit for 1.2.11 release
wget https://raw.githubusercontent.com/Homebrew/homebrew-core/6bfc8950c696d1f952425e8af2a6248603dc0df9/Formula/m/molten-vk.rb
/usr/local/bin/brew install -f --overwrite ./molten-vk.rb
export CXX=clang++
export CC=clang

export BREW_PATH;
BREW_PATH="$(brew --prefix)"
export BREW_X64_PATH;
BREW_X64_PATH="$("/usr/local/bin/brew" --prefix)"
export BREW_BIN="/usr/local/bin"
export BREW_SBIN="/usr/local/sbin"
export CMAKE_EXTRA_OPTS='-DLLVM_TARGETS_TO_BUILD=arm64'

export WORKDIR;
WORKDIR="$(pwd)"

# Get Qt
if [ ! -d "/tmp/Qt/$QT_VER" ]; then
  mkdir -p "/tmp/Qt"
  git clone https://github.com/engnr/qt-downloader.git
  cd qt-downloader
  git checkout f52efee0f18668c6d6de2dec0234b8c4bc54c597
  cd "/tmp/Qt"
  "$BREW_X64_PATH/bin/pipenv" run pip3 install py7zr requests semantic_version lxml
  mkdir -p "$QT_VER/macos" ; ln -s "macos" "$QT_VER/clang_64"
  "$BREW_X64_PATH/bin/pipenv" run "$WORKDIR/qt-downloader/qt-downloader" macos desktop "$QT_VER" clang_64 --opensource --addons qtmultimedia qtimageformats
fi

cd "$WORKDIR"
ditto "/tmp/Qt/$QT_VER" "qt-downloader/$QT_VER"

export Qt6_DIR="$WORKDIR/qt-downloader/$QT_VER/clang_64/lib/cmake/Qt$QT_VER_MAIN"
export SDL2_DIR="$BREW_ARM64_PATH/opt/sdl2/lib/cmake/SDL2"

export PATH="$BREW_X64_PATH/opt/llvm@$LLVM_COMPILER_VER/bin:$WORKDIR/qt-downloader/$QT_VER/clang_64/bin:$BREW_BIN:$BREW_SBIN:/usr/bin:/bin:/usr/sbin:/sbin:/opt/X11/bin:/Library/Apple/usr/bin:$PATH"
export LDFLAGS="-L$BREW_ARM64_PATH/lib $BREW_ARM64_PATH/opt/ffmpeg@5/lib/libavcodec.dylib $BREW_ARM64_PATH/opt/ffmpeg@5/lib/libavformat.dylib $BREW_ARM64_PATH/opt/ffmpeg@5/lib/libavutil.dylib $BREW_ARM64_PATH/opt/ffmpeg@5/lib/libswscale.dylib $BREW_ARM64_PATH/opt/ffmpeg@5/lib/libswresample.dylib $BREW_ARM64_PATH/opt/llvm@$LLVM_COMPILER_VER/lib/c++/libc++.1.dylib $BREW_ARM64_PATH/lib/libSDL2.dylib $BREW_ARM64_PATH/lib/libGLEW.dylib $BREW_ARM64_PATH/opt/llvm@$LLVM_COMPILER_VER/lib/libunwind.1.dylib -Wl,-rpath,$BREW_ARM64_PATH/lib"
export CPPFLAGS="-I$BREW_ARM64_PATH/include -I$BREW_X64_PATH/include -no-pie -D__MAC_OS_X_VERSION_MIN_REQUIRED=130000"
export CFLAGS="-D__MAC_OS_X_VERSION_MIN_REQUIRED=130000"
export LIBRARY_PATH="$BREW_ARM64_PATH/lib"
export LD_LIBRARY_PATH="$BREW_ARM64_PATH/lib"

export VULKAN_SDK
VULKAN_SDK="$BREW_ARM64_PATH/opt/molten-vk"
ln -s "$VULKAN_SDK/lib/libMoltenVK.dylib" "$VULKAN_SDK/lib/libvulkan.dylib" || true
export VK_ICD_FILENAMES="$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"

export LLVM_DIR
LLVM_DIR="$BREW_ARM64_PATH/opt/llvm@$LLVM_COMPILER_VER"
# exclude ffmpeg, LLVM, and sdl from submodule update
# shellcheck disable=SC2046
git submodule -q update --init --depth=1 --jobs=8 $(awk '/path/ && !/ffmpeg/ && !/llvm/ && !/SDL/ { print $3 }' .gitmodules)

# 3rdparty fixes
sed -i '' "s/extern const double NSAppKitVersionNumber;/const double NSAppKitVersionNumber = 1343;/g" 3rdparty/hidapi/hidapi/mac/hid.c

rm -rf build
mkdir build && cd build || exit 1

export MACOSX_DEPLOYMENT_TARGET=13.0

"$BREW_X64_PATH/bin/cmake" .. \
    -DUSE_SDL=ON \
    -DUSE_DISCORD_RPC=OFF \
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
    -DLLVM_TARGET_ARCH=arm64 \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_IGNORE_PATH="$BREW_X64_PATH/lib" \
    -DCMAKE_IGNORE_PREFIX_PATH=/usr/local/opt \
    -DCMAKE_SYSTEM_PROCESSOR=arm64 \
    -DCMAKE_TOOLCHAIN_FILE=buildfiles/cmake/TCDarwinARM64.cmake \
    -DCMAKE_CXX_FLAGS="-D__MAC_OS_X_VERSION_MIN_REQUIRED=130000" \
    -G Ninja

"$BREW_PATH/bin/ninja"; build_status=$?;

cd ..

{   [ "$CI_HAS_ARTIFACTS" = "true" ];
} && SHOULD_DEPLOY="true" || SHOULD_DEPLOY="false"

if [ "$build_status" -eq 0 ] && [ "$SHOULD_DEPLOY" = "true" ]; then
    .ci/deploy-mac-arm64.sh
fi
