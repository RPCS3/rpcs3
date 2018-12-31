#!/bin/env bash -ex
shopt -s nocasematch
cd build
if [ "$DEPLOY_APPIMAGE" = "true" ]; then
	DESTDIR=appdir ninja install
	curl -sLO "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
	chmod a+x linuxdeployqt*.AppImage
	./linuxdeployqt*.AppImage --appimage-extract
	./squashfs-root/AppRun ./appdir/usr/share/applications/*.desktop -bundle-non-qt-libs
	ls ./appdir/usr/lib/
	rm -r ./appdir/usr/share/doc
	rm ./appdir/usr/lib/libxcb*
	cp $(readlink -f /lib/x86_64-linux-gnu/libnsl.so.1) ./appdir/usr/lib/libnsl.so.1
	export PATH=/rpcs3/build/squashfs-root/usr/bin/:${PATH}

	# Embed newer libstdc++ for distros that don't come with it (ubuntu 14.04, 16.04)
	mkdir -p appdir/usr/optional/ ; mkdir -p appdir/usr/optional/libstdc++/
	cp /usr/lib/x86_64-linux-gnu/libstdc++.so.6 ./appdir/usr/optional/libstdc++/
	rm ./appdir/AppRun
	curl -sL https://github.com/RPCS3/AppImageKit-checkrt/releases/download/continuous2/AppRun-patched-x86_64 -o ./appdir/AppRun
	chmod a+x ./appdir/AppRun
	curl -sL https://github.com/RPCS3/AppImageKit-checkrt/releases/download/continuous2/exec-x86_64.so -o ./appdir/usr/optional/exec.so
	
	# Compile checker binary for AppImageKit-checkrt
	
	# This may need updating if you update the compiler or rpcs3 uses newer c++ features
	# See https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/config/abi/pre/gnu.ver
	# for which definitions correlate to which CXXABI version.
	printf "#include <memory>\nint main(){std::make_exception_ptr(0);}" | $CXX -x c++ -o ./appdir/usr/optional/checker -
	
	# Package it up and send it off
	./squashfs-root/usr/bin/appimagetool /rpcs3/build/appdir
	ls
	COMM_TAG="$(git describe --tags $(git rev-list --tags --max-count=1))"
	COMM_COUNT="$(git rev-list --count HEAD)"
	curl -sLO https://github.com/hcorion/uploadtool/raw/master/upload.sh
	
	mv ./RPCS3*.AppImage rpcs3-${COMM_TAG}-${COMM_COUNT}-${TRAVIS_COMMIT:0:8}_linux64.AppImage
	
	FILESIZEMB=$(bc <<< "scale=6; $(stat -c %s ./rpcs3*.AppImage)/1000000")
	SHA256SUM=($(sha256sum ./rpcs3*.AppImage))
	
	unset TRAVIS_REPO_SLUG
	REPO_SLUG=RPCS3/rpcs3-binaries-linux \
		UPLOADTOOL_BODY="$SHA256SUM;${FILESIZEMB}MB"\
		RELEASE_NAME=build-${TRAVIS_COMMIT}\
		RELEASE_TITLE=${COMM_TAG}-${COMM_COUNT}\
		REPO_COMMIT=d812f1254a1157c80fd402f94446310560f54e5f\
		bash upload.sh rpcs3*.AppImage
fi
if [ "$DEPLOY_PPA" = "true" ]; then
	export DEBFULLNAME="RPCS3 Build Bot"
fi
