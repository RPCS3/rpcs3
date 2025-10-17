#!/bin/sh -e

echo "Starting RPCS3 build (Bash script)"

# Automatically find clang_rt.builtins-x86_64.lib
echo "Searching for clang_rt.builtins-x86_64.lib ..."
clangBuiltinsLibPath=$(find "C:\Program Files\LLVM\lib\clang" -name "clang_rt.builtins-x86_64.lib" | sed 's|Program Files|PROGRA~1|g')

if [ -z "$clangBuiltinsLibPath" ]; then
    echo "ERROR: Could not find clang_rt.builtins-x86_64.lib in LLVM installation."
    exit 1
fi

clangBuiltinsDir=$(dirname "$clangBuiltinsLibPath")
clangBuiltinsLib=$(basename "$clangBuiltinsLibPath")
# shellcheck disable=SC2028 
clangPath=$(echo "C:\Program Files\LLVM\bin" | sed 's|Program Files|PROGRA~1|g')

echo "Found Clang builtins library: $clangBuiltinsLib in $clangBuiltinsDir"
echo "Found Clang Path: $clangPath"

# Search for mt.exe in SDK bin directories
echo "Searching for llvm-mt.exe ..."
mtPath=$(find "$clangPath" -name "llvm-mt.exe")

if [ -z "$mtPath" ]; then
    echo "ERROR: Could not find llvm-mt.exe in SDK directories."
    exit 1
fi

echo "Found llvm-mt.exe at: $mtPath"

VcpkgRoot="$(pwd)/vcpkg"
VcpkgTriplet="$VCPKG_TRIPLET"
VcpkgInstall="$VcpkgRoot/installed/$VcpkgTriplet"
VcpkgInclude="$VcpkgInstall/include"
VcpkgLib="$VcpkgInstall/lib"

# Configure git safe directory
echo "Configuring git safe directory"
git config --global --add safe.directory '*'

# Initialize submodules except certain ones
echo "Initializing submodules"
# shellcheck disable=SC2046
git submodule -q update --init $(awk '/path/ && !/llvm/ && !/opencv/ && !/FAudio/ && !/libpng/ && !/zlib/ && !/feralinteractive/ { print $3 }' .gitmodules)

# Create and enter build directory
echo "Creating build directory"
mkdir -p build
cd build || exit
echo "Changed directory to: $(pwd)"

# Run CMake with Ninja generator and required flags
echo "Running CMake configuration"
cmake .. \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER="${clangPath}/clang-cl.exe" \
    -DCMAKE_CXX_COMPILER="${clangPath}/clang-cl.exe" \
    -DCMAKE_LINKER="${clangPath}/lld-link.exe" \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot/scripts/buildsystems/vcpkg.cmake" \
    -DCMAKE_EXE_LINKER_FLAGS="/LIBPATH:${clangBuiltinsDir} /defaultlib:${clangBuiltinsLib}" \
    -DCMAKE_MT="${mtPath}" \
    -DUSE_NATIVE_INSTRUCTIONS=OFF \
    -DUSE_PRECOMPILED_HEADERS=OFF \
    -DVCPKG_TARGET_TRIPLET="$VcpkgTriplet" \
    -DFFMPEG_INCLUDE_DIR="$VcpkgInclude" \
    -DFFMPEG_LIBAVCODEC="$VcpkgLib/avcodec.lib" \
    -DFFMPEG_LIBAVFORMAT="$VcpkgLib/avformat.lib" \
    -DFFMPEG_LIBAVUTIL="$VcpkgLib/avutil.lib" \
    -DFFMPEG_LIBSWSCALE="$VcpkgLib/swscale.lib" \
    -DFFMPEG_LIBSWRESAMPLE="$VcpkgLib/swresample.lib" \
    -DUSE_SYSTEM_CURL=OFF \
    -DUSE_FAUDIO=OFF \
    -DUSE_SDL=ON \
    -DUSE_SYSTEM_SDL=OFF \
    -DUSE_SYSTEM_FFMPEG=ON \
    -DUSE_SYSTEM_OPENCV=ON \
    -DUSE_SYSTEM_OPENAL=OFF \
    -DUSE_SYSTEM_LIBPNG=ON \
    -DUSE_DISCORD_RPC=ON \
    -DUSE_SYSTEM_ZSTD=ON \
    -DWITH_LLVM=ON \
    -DSTATIC_LINK_LLVM=ON \
    -DBUILD_RPCS3_TESTS=OFF

echo "CMake configuration complete"

# Build with ninja
echo "Starting build with Ninja..."
ninja

echo "Build succeeded"

# Go back to root directory
cd ..
echo "Returned to root directory: $(pwd)"
