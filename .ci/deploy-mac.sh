#!/bin/sh -ex

# shellcheck disable=SC2086
cd build || exit 1

cd bin
mkdir -p "rpcs3.app/Contents/Resources/vulkan/icd.d" || true
wget https://github.com/KhronosGroup/MoltenVK/releases/download/v1.4.1/MoltenVK-macos-privateapi.tar
tar -xvf MoltenVK-macos-privateapi.tar
cp "MoltenVK/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib" "rpcs3.app/Contents/Frameworks/libMoltenVK.dylib"
cp "MoltenVK/MoltenVK/dynamic/dylib/macOS/MoltenVK_icd.json" "rpcs3.app/Contents/Resources/vulkan/icd.d/MoltenVK_icd.json"
sed -i '' "s/.\//..\/..\/..\/Frameworks\//g" "rpcs3.app/Contents/Resources/vulkan/icd.d/MoltenVK_icd.json"

cp "$(realpath $BREW_PATH/opt/llvm@$LLVM_COMPILER_VER/lib/c++/libc++abi.1.0.dylib)" "rpcs3.app/Contents/Frameworks/libc++abi.1.dylib"
cp "$(realpath $BREW_PATH/opt/gcc/lib/gcc/current/libgcc_s.1.1.dylib)" "rpcs3.app/Contents/Frameworks/libgcc_s.1.1.dylib"

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

# Copy Qt translations manually
QT_TRANS="$WORKDIR/qt-downloader/$QT_VER/clang_64/translations"
cp $QT_TRANS/qt_*.qm rpcs3.app/Contents/translations
cp $QT_TRANS/qtbase_*.qm rpcs3.app/Contents/translations
cp $QT_TRANS/qtmultimedia_*.qm rpcs3.app/Contents/translations
rm -f rpcs3.app/Contents/translations/qt_help_*.qm

# Need to do this rename hack due to case insensitive filesystem
mv rpcs3.app RPCS3_.app
mv RPCS3_.app RPCS3.app

# Hack
install_name_tool -delete_rpath /opt/homebrew/lib RPCS3.app/Contents/MacOS/rpcs3 || true
install_name_tool -delete_rpath /usr/local/lib RPCS3.app/Contents/MacOS/rpcs3 || true

# NOTE: "--deep" is deprecated
codesign --deep -fs - RPCS3.app

echo "[InternetShortcut]" > Quickstart.url
echo "URL=https://rpcs3.net/quickstart" >> Quickstart.url
echo "IconIndex=0" >> Quickstart.url

if [ "$AARCH64" -eq 1 ]; then
  ARCHIVE_FILEPATH="$BUILD_ARTIFACTSTAGINGDIRECTORY/rpcs3-v${LVER}_macos_aarch64.7z"
else
  ARCHIVE_FILEPATH="$BUILD_ARTIFACTSTAGINGDIRECTORY/rpcs3-v${LVER}_macos.7z"
fi
7z a -mx9 "$ARCHIVE_FILEPATH" RPCS3.app Quickstart.url
FILESIZE=$(stat -f %z "$ARCHIVE_FILEPATH")
SHA256SUM=$(shasum -a 256 "$ARCHIVE_FILEPATH" | awk '{ print $1 }')

cd ..
echo "${SHA256SUM};${FILESIZE}B" > "$RELEASE_MESSAGE"
cd bin
