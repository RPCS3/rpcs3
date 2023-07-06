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

# check if we are on ARM or x86_64
if [ "$BUILDING_FOR" = "arm64" ]; then
	cp "/opt/homebrew/opt/llvm@16/lib/c++/libc++abi.1.0.dylib" "rpcs3.app/Contents/lib/libc++abi.1.dylib"
		
# we have ffmpeg libs bundled in the app bundle in rpcs3.app/Contents/Frameworks , now we need to change the rpath to point to it
# this is needed because we are using a homebrewed ffmpeg for arm64, and the rpath points to the system ffmpeg
# for example we want to point to rpcs3.app/Contents/Frameworks/libavcodec.60.dylib
	install_name_tool -change "/opt/homebrew/Cellar/ffmpeg/6.0/lib/libavcodec.60.dylib" "@executable_path/../Frameworks/libavcodec.60.dylib" "rpcs3.app/Contents/MacOS/rpcs3"
	install_name_tool -change "/opt/homebrew/Cellar/ffmpeg/6.0/lib/libavdevice.60.dylib" "@executable_path/../Frameworks/libavdevice.60.dylib" "rpcs3.app/Contents/MacOS/rpcs3"
	install_name_tool -change "/opt/homebrew/Cellar/ffmpeg/6.0/lib/libavfilter.7.dylib" "@executable_path/../Frameworks/libavfilter.7.dylib" "rpcs3.app/Contents/MacOS/rpcs3"
	install_name_tool -change "/opt/homebrew/Cellar/ffmpeg/6.0/lib/libavformat.60.dylib" "@executable_path/../Frameworks/libavformat.60.dylib" "rpcs3.app/Contents/MacOS/rpcs3"
	install_name_tool -change "/opt/homebrew/Cellar/ffmpeg/6.0/lib/libavutil.58.dylib" "@executable_path/../Frameworks/libavutil.58.dylib" "rpcs3.app/Contents/MacOS/rpcs3"
	install_name_tool -change "/opt/homebrew/Cellar/ffmpeg/6.0/lib/libswresample.4.dylib" "@executable_path/../Frameworks/libswresample.4.dylib" "rpcs3.app/Contents/MacOS/rpcs3"
	install_name_tool -change "/opt/homebrew/Cellar/ffmpeg/6.0/lib/libswscale.7.dylib" "@executable_path/../Frameworks/libswscale.7.dylib" "rpcs3.app/Contents/MacOS/rpcs3"

else
	cp "/usr/local/opt/llvm@16/lib/c++/libc++abi.1.0.dylib" "rpcs3.app/Contents/lib/libc++abi.1.dylib"

	# change the path to app for sdl2
	install_name_tool -change "/opt/homebrew/opt/sdl2/lib/libSDL2-2.0.0.dylib" "@executable_path/../Frameworks/libSDL2-2.0.0.dylib" "rpcs3.app/Contents/MacOS/rpcs3"
	
	# now faudio 
	install_name_tool -change "/opt/homebrew/opt/faudio/lib/libFAudio.0.dylib" "@executable_path/../Frameworks/libFAudio.0.dylib" "rpcs3.app/Contents/MacOS/rpcs3"
 
fi
rm -rf "rpcs3.app/Contents/Frameworks/QtPdf.framework" \
"rpcs3.app/Contents/Frameworks/QtQml.framework" \
"rpcs3.app/Contents/Frameworks/QtQmlModels.framework" \
"rpcs3.app/Contents/Frameworks/QtQuick.framework" \
"rpcs3.app/Contents/Frameworks/QtVirtualKeyboard.framework" \
"rpcs3.app/Contents/Plugins/platforminputcontexts" \
"rpcs3.app/Contents/Plugins/virtualkeyboard" \
"rpcs3.app/Contents/Resources/git"


# codesign it 
codesign --force --deep --sign - "rpcs3.app/Contents/MacOS/rpcs3"
# Need to do this rename hack due to case insensitive filesystem
mv rpcs3.app RPCS3_.app
mv RPCS3_.app RPCS3.app

echo "[InternetShortcut]" > Quickstart.url
echo "URL=https://rpcs3.net/quickstart" >> Quickstart.url
echo "IconIndex=0" >> Quickstart.url

DMG_FILEPATH="$BUILD_ARTIFACTSTAGINGDIRECTORY/rpcs3-v${COMM_TAG}-${COMM_COUNT}-${COMM_HASH}_macos.dmg"

/opt/homebrew/bin/create-dmg --volname RPCS3 \
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

/opt/homebrew/bin/7z a -mx9 rpcs3-v"${COMM_TAG}"-"${COMM_COUNT}"-"${COMM_HASH}"_macos.7z RPCS3.app

FILESIZE=$(stat -f %z "$DMG_FILEPATH")
SHA256SUM=$(shasum -a 256 "$DMG_FILEPATH" | awk '{ print $1 }')
cd ..
echo "${SHA256SUM};${FILESIZE}B" > "$RELEASE_MESSAGE"
cd bin

mv ./rpcs3*_macos.7z "$ARTDIR"
