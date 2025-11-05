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
mkdir "rpcs3.app/Contents/lib/" || true

cp "$(realpath /opt/homebrew/opt/llvm@$LLVM_COMPILER_VER/lib/c++/libc++abi.1.0.dylib)" "rpcs3.app/Contents/Frameworks/libc++abi.1.dylib"
cp "$(realpath /opt/homebrew/lib/libsharpyuv.0.dylib)" "rpcs3.app/Contents/lib/libsharpyuv.0.dylib"
cp "$(realpath /opt/homebrew/lib/libintl.8.dylib)" "rpcs3.app/Contents/lib/libintl.8.dylib"

rm -rf "rpcs3.app/Contents/Frameworks/QtPdf.framework" \
"rpcs3.app/Contents/Frameworks/QtQml.framework" \
"rpcs3.app/Contents/Frameworks/QtQmlModels.framework" \
"rpcs3.app/Contents/Frameworks/QtQuick.framework" \
"rpcs3.app/Contents/Frameworks/QtVirtualKeyboard.framework" \
"rpcs3.app/Contents/Plugins/platforminputcontexts" \
"rpcs3.app/Contents/Plugins/virtualkeyboard" \
"rpcs3.app/Contents/Resources/git"

../../.ci/optimize-mac.sh rpcs3.app

# Download translations
mkdir -p "rpcs3.app/Contents/translations"
ZIP_URL=$(curl -fsSL "https://api.github.com/repos/RPCS3/rpcs3_translations/releases/latest" \
  | grep "browser_download_url" \
  | grep "RPCS3-languages.zip" \
  | cut -d '"' -f 4)
if [ -z "$ZIP_URL" ]; then
  echo "Failed to find RPCS3-languages.zip in the latest release. Continuing without translations."
else
  echo "Downloading translations from: $ZIP_URL"
  curl -L -o translations.zip "$ZIP_URL" || {
    echo "Failed to download translations.zip. Continuing without translations."
    exit 0
  }
  unzip -o translations.zip -d "rpcs3.app/Contents/translations" >/dev/null 2>&1 || \
    echo "Failed to extract translations.zip. Continuing without translations."
  rm -f translations.zip
fi

# Hack
install_name_tool -delete_rpath /opt/homebrew/lib RPCS3.app/Contents/MacOS/rpcs3 || echo "Hack for deleting rpath /opt/homebrew/lib not needed"
install_name_tool -delete_rpath /opt/homebrew/opt/llvm@$LLVM_COMPILER_VER/lib RPCS3.app/Contents/MacOS/rpcs3 || echo "Hack for deleting rpath /opt/homebrew/opt/llvm@$LLVM_COMPILER_VER/lib not needed"

# Need to do this rename hack due to case insensitive filesystem
mv rpcs3.app RPCS3_.app
mv RPCS3_.app RPCS3.app

# NOTE: "--deep" is deprecated
codesign --deep -fs - RPCS3.app

echo "[InternetShortcut]" > Quickstart.url
echo "URL=https://rpcs3.net/quickstart" >> Quickstart.url
echo "IconIndex=0" >> Quickstart.url

ARCHIVE_FILEPATH="$BUILD_ARTIFACTSTAGINGDIRECTORY/rpcs3-v${COMM_TAG}-${COMM_COUNT}-${COMM_HASH}_macos_arm64.7z"
"/opt/homebrew/bin/7z" a -mx9 "$ARCHIVE_FILEPATH" RPCS3.app Quickstart.url
FILESIZE=$(stat -f %z "$ARCHIVE_FILEPATH")
SHA256SUM=$(shasum -a 256 "$ARCHIVE_FILEPATH" | awk '{ print $1 }')

cd ..
echo "${SHA256SUM};${FILESIZE}B" > "$RELEASE_MESSAGE"
cd bin
