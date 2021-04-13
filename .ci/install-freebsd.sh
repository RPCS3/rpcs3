#!/usr/bin/env -S su -m root -ex
# NOTE: this script is run under root permissions
# shellcheck shell=sh disable=SC2096

# RPCS3 often needs recent Qt5 and Vulkan-Headers
sed -i '' 's/quarterly/latest/' /etc/pkg/FreeBSD.conf

export ASSUME_ALWAYS_YES=true
pkg info # debug

# Prefer newer Clang than in base system (see also .ci/build-freebsd.sh)
pkg install llvm12

# Mandatory dependencies (qt5-dbus and qt5-gui are pulled via qt5-widgets)
pkg install git ccache cmake ninja qt5-qmake qt5-buildtools qt5-widgets qt5-concurrent glew openal-soft ffmpeg

# Optional dependencies (libevdev is pulled by qt5-gui)
pkg install pkgconf alsa-lib pulseaudio sdl2 evdev-proto vulkan-headers vulkan-loader
