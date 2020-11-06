#!/usr/bin/env -S su -m root -ex
# NOTE: this script is run under root permissions
# shellcheck shell=sh disable=SC2096

# RPCS3 often needs recent Qt5 and Vulkan-Headers
sed -i '' 's/quarterly/latest/' /etc/pkg/FreeBSD.conf

export ASSUME_ALWAYS_YES=true
pkg info # debug

# Prefer newer Clang on TravisCI (see also .ci/install-freebsd.sh)
if [ -n "$TRAVIS" ]; then
	pkg install llvm11
fi

# Mandatory dependencies (qt5-dbus and qt5-gui are pulled via qt5-widgets)
pkg install git ccache cmake ninja qt5-qmake qt5-buildtools qt5-widgets qt5-concurrent glew openal-soft ffmpeg

# Optional dependencies (libevdev is pulled by qt5-gui)
pkg install pkgconf alsa-lib pulseaudio sdl2 evdev-proto vulkan-headers vulkan-loader
