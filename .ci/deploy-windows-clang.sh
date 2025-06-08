#!/bin/sh -ex

# source ci-vars.env
# shellcheck disable=SC1091
. .ci/ci-vars.env

cd build || exit 1

CPU_ARCH="${1:-x86_64}"

echo "Deploying rpcs3 windows clang $CPU_ARCH"

# BUILD_blablabla is CI specific, so we wrap it for portability
ARTIFACT_DIR=$(cygpath -u "$BUILD_ARTIFACTSTAGINGDIRECTORY")
MSYS2_CLANG_BIN=$(cygpath -w /clang64/bin)
MSYS2_USR_BIN=$(cygpath -w /usr/bin)

echo "Installing dependencies of: ./bin/rpcs3.exe (MSYS2 dir is '$MSYS2_CLANG_BIN', usr dir is '$MSYS2_USR_BIN')"
cmake -DMSYS2_CLANG_BIN="$MSYS2_CLANG_BIN" -DMSYS2_USR_BIN="$MSYS2_USR_BIN" -Dexe=./bin/rpcs3.exe -P ../buildfiles/cmake/CopyRuntimeDependencies.cmake

# Prepare compatibility and SDL database for packaging
mkdir ./bin/config
mkdir ./bin/config/input_configs
curl -fsSL 'https://raw.githubusercontent.com/gabomdq/SDL_GameControllerDB/master/gamecontrollerdb.txt' 1> ./bin/config/input_configs/gamecontrollerdb.txt
curl -fsSL 'https://rpcs3.net/compatibility?api=v1&export' | iconv -t UTF-8 1> ./bin/GuiConfigs/compat_database.dat

# Package artifacts
7z a -m0=LZMA2 -mx9 "$BUILD" ./bin/*

# Generate sha256 hashes
# Write to file for GitHub releases
sha256sum "$BUILD" | awk '{ print $1 }' | tee "$BUILD.sha256"
echo "$(cat "$BUILD.sha256");$(stat -c %s "$BUILD")B" > GitHubReleaseMessage.txt

# Move files to publishing directory
mkdir -p "$ARTIFACT_DIR"
cp -- "$BUILD" "$ARTIFACT_DIR"
cp -- "$BUILD.sha256" "$ARTIFACT_DIR"
