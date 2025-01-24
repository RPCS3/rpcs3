#!/bin/sh -ex

# shellcheck disable=SC2086
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

cp "/usr/local/opt/llvm@$LLVM_COMPILER_VER/lib/c++/libc++abi.1.0.dylib" "rpcs3.app/Contents/lib/libc++abi.1.dylib"
cp "$(realpath /usr/local/lib/libsharpyuv.0.dylib)" "rpcs3.app/Contents/lib/libsharpyuv.0.dylib"
cp "$(realpath /usr/local/lib/libintl.8.dylib)" "rpcs3.app/Contents/lib/libintl.8.dylib"

rm -rf "rpcs3.app/Contents/Frameworks/QtPdf.framework" \
"rpcs3.app/Contents/Frameworks/QtQml.framework" \
"rpcs3.app/Contents/Frameworks/QtQmlModels.framework" \
"rpcs3.app/Contents/Frameworks/QtQuick.framework" \
"rpcs3.app/Contents/Frameworks/QtVirtualKeyboard.framework" \
"rpcs3.app/Contents/Plugins/platforminputcontexts" \
"rpcs3.app/Contents/Plugins/virtualkeyboard" \
"rpcs3.app/Contents/Resources/git"

../../.ci/optimize-mac.sh rpcs3.app

# Need to do this rename hack due to case insensitive filesystem
mv rpcs3.app RPCS3_.app
mv RPCS3_.app RPCS3.app

# Hack
install_name_tool \
-delete_rpath /usr/local/lib \
-delete_rpath /usr/local/opt/llvm@$LLVM_COMPILER_VER/lib RPCS3.app/Contents/MacOS/rpcs3
#-delete_rpath /usr/local/Cellar/sdl3/3.2.8/lib

# NOTE: "--deep" is deprecated
codesign --deep -fs - RPCS3.app

echo "[InternetShortcut]" > Quickstart.url
echo "URL=https://rpcs3.net/quickstart" >> Quickstart.url
echo "IconIndex=0" >> Quickstart.url

#DMG_FILEPATH="$BUILD_ARTIFACTSTAGINGDIRECTORY/rpcs3-v${COMM_TAG}-${COMM_COUNT}-${COMM_HASH}_macos.dmg"
#"$BREW_X64_PATH/bin/create-dmg" --volname RPCS3 \
#--window-size 800 400 \
#--icon-size 100 \
#--icon rpcs3.app 200 190 \
#--add-file Quickstart.url Quickstart.url 400 20 \
#--hide-extension rpcs3.app \
#--hide-extension Quickstart.url \
#--app-drop-link 600 185 \
#--skip-jenkins \
#--format ULMO \
#"$DMG_FILEPATH" \
#RPCS3.app
#FILESIZE=$(stat -f %z "$DMG_FILEPATH")
#SHA256SUM=$(shasum -a 256 "$DMG_FILEPATH" | awk '{ print $1 }')

ARCHIVE_FILEPATH="$BUILD_ARTIFACTSTAGINGDIRECTORY/rpcs3-v${COMM_TAG}-${COMM_COUNT}-${COMM_HASH}_macos.7z"
"$BREW_X64_PATH/bin/7z" a -mx9 "$ARCHIVE_FILEPATH" RPCS3.app Quickstart.url
FILESIZE=$(stat -f %z "$ARCHIVE_FILEPATH")
SHA256SUM=$(shasum -a 256 "$ARCHIVE_FILEPATH" | awk '{ print $1 }')

cd ..
echo "${SHA256SUM};${FILESIZE}B" > "$RELEASE_MESSAGE"
cd bin
