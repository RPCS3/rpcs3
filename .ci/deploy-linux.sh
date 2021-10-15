#!/bin/sh -ex

cd build || exit 1

if [ "$DEPLOY_APPIMAGE" = "true" ]; then
    DESTDIR=appdir ninja install
    QT_APPIMAGE="linuxdeployqt.AppImage"

    curl -sL "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage" > "$QT_APPIMAGE"
    chmod a+x "$QT_APPIMAGE"
    "./$QT_APPIMAGE" --appimage-extract
    ./squashfs-root/AppRun ./appdir/usr/share/applications/*.desktop -bundle-non-qt-libs
    ls ./appdir/usr/lib/
    rm -r ./appdir/usr/share/doc
    cp "$(readlink -f /lib/x86_64-linux-gnu/libnsl.so.1)" ./appdir/usr/lib/libnsl.so.1
    export PATH=/rpcs3/build/squashfs-root/usr/bin/:${PATH}

    # Embed newer libstdc++ for distros that don't come with it (ubuntu 16.04)
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
    # Currently we target a minimum of GLIBCXX_3.4.29 and CXXABI_1.3.11
    printf "#include <sstream>\n#include <exception>\n#include <memory_resource>\nint main(){auto x = std::stringbuf();x.get_allocator();std::make_exception_ptr(0);std::pmr::get_default_resource();}" \
     | $CXX -x c++ -std=c++2a -o ./appdir/usr/optional/checker -

    # Package it up and send it off
    ./squashfs-root/usr/bin/appimagetool "$APPDIR"
    
    ls

    COMM_TAG="$(grep 'version{.*}' ../rpcs3/rpcs3_version.cpp | awk -F[\{,] '{printf "%d.%d.%d", $2, $3, $4}')"
    COMM_COUNT="$(git rev-list --count HEAD)"
    COMM_HASH="$(git rev-parse --short=8 HEAD)"
    RPCS3_APPIMAGE="rpcs3-v${COMM_TAG}-${COMM_COUNT}-${COMM_HASH}_linux64.AppImage"

    mv ./RPCS3*.AppImage "$RPCS3_APPIMAGE"

    # If we're building using a CI, let's copy over the AppImage artifact
    if [ -n "$BUILD_ARTIFACTSTAGINGDIRECTORY" ]; then
        cp "$RPCS3_APPIMAGE" "$ARTDIR"
    fi

    FILESIZE=$(stat -c %s ./rpcs3*.AppImage)
    SHA256SUM=$(sha256sum ./rpcs3*.AppImage | awk '{ print $1 }')
    echo "${SHA256SUM};${FILESIZE}B" > "$RELEASE_MESSAGE"
    
fi

if [ "$DEPLOY_PPA" = "true" ]; then
    export DEBFULLNAME="RPCS3 Build Bot"
fi
