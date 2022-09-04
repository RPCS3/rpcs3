#!/bin/sh -ex

cd build || exit 1

# Gather explicit version number and number of commits
COMM_TAG=$(awk '/version{.*}/ { printf("%d.%d.%d", $5, $6, $7) }' ../rpcs3/rpcs3_version.cpp)
COMM_COUNT=$(git rev-list --count HEAD)
COMM_HASH=$(git rev-parse --short=8 HEAD)

AVVER="${COMM_TAG}-${COMM_COUNT}"

# AVVER is used for GitHub releases, it is the version number.
echo "AVVER=$AVVER" >> ../.ci/ci-vars.env

cd bin
mkdir "rpcs3.app/Contents/lib/"

ARCH_NAME="$(uname -m)"
if [ "${ARCH_NAME}" = "arm64" ]; then
  cp "/opt/homebrew/opt/llvm@13/lib/libc++abi.1.0.dylib" "rpcs3.app/Contents/lib/libc++abi.1.dylib"
else
  cp "/usr/local/opt/llvm@13/lib/libc++abi.1.0.dylib" "rpcs3.app/Contents/lib/libc++abi.1.dylib"
fi

rm -rf "rpcs3.app/Contents/Frameworks/QtPdf.framework" \
"rpcs3.app/Contents/Frameworks/QtQml.framework" \
"rpcs3.app/Contents/Frameworks/QtQmlModels.framework" \
"rpcs3.app/Contents/Frameworks/QtQuick.framework" \
"rpcs3.app/Contents/Frameworks/QtVirtualKeyboard.framework" \
"rpcs3.app/Contents/Plugins/platforminputcontexts" \
"rpcs3.app/Contents/Plugins/virtualkeyboard"

# Need to do this rename hack due to case insensitive filesystem
mv rpcs3.app RPCS3_.app
mv RPCS3_.app RPCS3.app

echo "[InternetShortcut]" > Quickstart.url
echo "URL=https://rpcs3.net/quickstart" >> Quickstart.url
echo "IconIndex=0" >> Quickstart.url

DMG_FILEPATH="$BUILD_ARTIFACTSTAGINGDIRECTORY/rpcs3-v${COMM_TAG}-${COMM_COUNT}-${COMM_HASH}_macos_${ARCH_NAME}.dmg"

create-dmg --volname RPCS3 \
--window-size 800 400 \
--icon-size 100 \
--icon rpcs3.app 200 190 \
--add-file Quickstart.url Quickstart.url 400 20 \
--hide-extension rpcs3.app \
--hide-extension Quickstart.url \
--app-drop-link 600 185 \
--skip-jenkins \
"$DMG_FILEPATH" \
RPCS3.app

7z a -mx9 rpcs3-v"${COMM_TAG}"-"${COMM_COUNT}"-"${COMM_HASH}"_macos_"${ARCH_NAME}".7z RPCS3.app

FILESIZE=$(stat -f %z "$DMG_FILEPATH")
SHA256SUM=$(shasum -a 256 "$DMG_FILEPATH" | awk '{ print $1 }')
cd ..
echo "${SHA256SUM};${FILESIZE}B" > "$RELEASE_MESSAGE"
cd bin

mv ./rpcs3*_macos_*.7z "$ARTDIR"
