#!/bin/sh -ex

# First let's print some info about our caches
"$(cygpath -u "$CCACHE_BIN_DIR")"/ccache.exe --show-stats -v

# BUILD_blablabla is Azure specific, so we wrap it for portability
ARTIFACT_DIR="$BUILD_ARTIFACTSTAGINGDIRECTORY"
BUILD="llvmlibs_mt.7z"

# Package artifacts
7z a -m0=LZMA2 -mx9 "$BUILD" ./build/lib/Release-x64/llvm_build

# Generate sha256 hashes
# Write to file for GitHub releases
sha256sum "$BUILD" | awk '{ print $1 }' | tee "$BUILD.sha256"
echo "$(cat "$BUILD.sha256");$(stat -c %s "$BUILD")B" > GitHubReleaseMessage.txt

# Move files to publishing directory
cp -- "$BUILD" "$ARTIFACT_DIR"
cp -- "$BUILD.sha256" "$ARTIFACT_DIR"
