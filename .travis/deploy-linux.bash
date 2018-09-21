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
	export PATH=/rpcs3/build/squashfs-root/usr/bin/:${PATH}

	# Embed newer libstdc++ for distros that don't come with it (ubuntu 14.04, 16.04)
	mkdir -p appdir/usr/optional/ ; mkdir -p appdir/usr/optional/libstdc++/
	cp /usr/lib/x86_64-linux-gnu/libstdc++.so.6 ./appdir/usr/optional/libstdc++/
	rm ./appdir/AppRun
	curl -sL https://github.com/RPCS3/AppImageKit-checkrt/releases/download/continuous/AppRun-patched-x86_64 -o ./appdir/AppRun
	chmod a+x ./appdir/AppRun
	curl -sL https://github.com/RPCS3/AppImageKit-checkrt/releases/download/continuous/exec-x86_64.so -o ./appdir/usr/optional/exec.so

	# Package it up and send it off
	./squashfs-root/usr/bin/appimagetool /rpcs3/build/appdir
	ls
	COMM_TAG="$(git describe --tags $(git rev-list --tags --max-count=1))"
	COMM_COUNT="$(git rev-list --count HEAD)"
	curl "${UPLOAD_URL}${TRAVIS_COMMIT:0:8}&t=${COMM_TAG}&a=${COMM_COUNT}" --upload-file ./RPCS3*.AppImage
fi
if [ "$DEPLOY_PPA" = "true" ]; then
	export DEBFULLNAME="RPCS3 Build Bot"
fi
