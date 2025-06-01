#!/bin/sh -ex

cd build || exit 1

CPU_ARCH="${1:-x86_64}"

if [ "$DEPLOY_APPIMAGE" = "true" ]; then
    DESTDIR=AppDir ninja install

    curl -fsSLo /usr/bin/linuxdeploy "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$CPU_ARCH.AppImage"
    chmod +x /usr/bin/linuxdeploy
    curl -fsSLo /usr/bin/linuxdeploy-plugin-qt "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$CPU_ARCH.AppImage"
    chmod +x /usr/bin/linuxdeploy-plugin-qt
    curl -fsSLo linuxdeploy-plugin-checkrt.sh https://github.com/darealshinji/linuxdeploy-plugin-checkrt/releases/download/continuous/linuxdeploy-plugin-checkrt.sh
    chmod +x ./linuxdeploy-plugin-checkrt.sh

    export EXTRA_PLATFORM_PLUGINS="libqwayland-egl.so;libqwayland-generic.so"
    export EXTRA_QT_PLUGINS="svg;wayland-decoration-client;wayland-graphics-integration-client;wayland-shell-integration;waylandcompositor"

    APPIMAGE_EXTRACT_AND_RUN=1 linuxdeploy --appdir AppDir --plugin qt --plugin checkrt

    # Remove libwayland-client because it has platform-dependent exports and breaks other OSes
    rm -f ./AppDir/usr/lib/libwayland-client.so*

    # Remove libvulkan because it causes issues with gamescope
    rm -f ./AppDir/usr/lib/libvulkan.so*

    # Remove unused Qt6 libraries
    rm -f ./AppDir/usr/lib/libQt6VirtualKeyboard.so*
    rm -f ./AppDir/usr/plugins/platforminputcontexts/libqtvirtualkeyboardplugin.so*

    # Remove git directory containing local commit history file
    rm -rf ./AppDir/usr/share/rpcs3/git

    curl -fsSLo /uruntime "https://github.com/VHSgunzo/uruntime/releases/download/v0.3.4/uruntime-appimage-dwarfs-$CPU_ARCH"
    chmod +x /uruntime
    /uruntime --appimage-mkdwarfs -f --set-owner 0 --set-group 0 --no-history --no-create-timestamp \
    --compression zstd:level=22 -S26 -B32 --header /uruntime -i AppDir -o RPCS3.AppImage

    APPIMAGE_SUFFIX="linux_${CPU_ARCH}"
    if [ "$CPU_ARCH" = "x86_64" ]; then
        # Preserve back compat. Previous versions never included the full arch.
        APPIMAGE_SUFFIX="linux64"
    fi

    COMM_TAG=$(awk '/version{.*}/ { printf("%d.%d.%d", $5, $6, $7) }' ../rpcs3/rpcs3_version.cpp)
    COMM_COUNT="$(git rev-list --count HEAD)"
    COMM_HASH="$(git rev-parse --short=8 HEAD)"
    RPCS3_APPIMAGE="rpcs3-v${COMM_TAG}-${COMM_COUNT}-${COMM_HASH}_${APPIMAGE_SUFFIX}.AppImage"

    mv ./RPCS3*.AppImage "$RPCS3_APPIMAGE"

    # If we're building using a CI, let's copy over the AppImage artifact
    if [ -n "$BUILD_ARTIFACTSTAGINGDIRECTORY" ]; then
        cp "$RPCS3_APPIMAGE" "$ARTDIR"
    fi

    FILESIZE=$(stat -c %s ./rpcs3*.AppImage)
    SHA256SUM=$(sha256sum ./rpcs3*.AppImage | awk '{ print $1 }')
    echo "${SHA256SUM};${FILESIZE}B" > "$RELEASE_MESSAGE"
fi
