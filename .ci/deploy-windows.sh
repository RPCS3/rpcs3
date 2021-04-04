#!/bin/sh -ex

# BUILD_blablabla is Azure specific, so we wrap it for portability
ARTIFACT_DIR="$BUILD_ARTIFACTSTAGINGDIRECTORY"

# Remove unecessary files
rm -f ./bin/rpcs3.exp ./bin/rpcs3.lib ./bin/rpcs3.pdb ./bin/vc_redist.x64.exe

# Prepare compatibility database for packaging, as well as
# certificate for ssl (auto-updater)
curl -sL 'https://rpcs3.net/compatibility?api=v1&export' | iconv -t UTF-8 > ./bin/GuiConfigs/compat_database.dat
curl -sL 'https://curl.haxx.se/ca/cacert.pem' > ./bin/cacert.pem

# Package artifacts
7z a -m0=LZMA2 -mx9 "$BUILD" ./bin/*

# Generate sha256 hashes
# Write to file for GitHub releases
sha256sum "$BUILD" | awk '{ print $1 }' | tee "$BUILD.sha256"
echo "$(cat "$BUILD.sha256");$(stat -c %s "$BUILD")B" > GitHubReleaseMessage.txt

# Move files to publishing directory
cp -- "$BUILD" "$ARTIFACT_DIR"
cp -- "$BUILD.sha256" "$ARTIFACT_DIR"
