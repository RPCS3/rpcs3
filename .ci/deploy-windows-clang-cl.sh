#!/bin/sh -ex

# source ci-vars.env
# shellcheck disable=SC1091
. .ci/ci-vars.env

cd build || exit 1

CPU_ARCH="${1:-x86_64}"

echo "Deploying rpcs3 windows clang-cl $CPU_ARCH"

# BUILD_blablabla is CI specific, so we wrap it for portability
ARTIFACT_DIR=$(cygpath -u "$BUILD_ARTIFACTSTAGINGDIRECTORY")

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
