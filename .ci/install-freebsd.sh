#!/usr/bin/env -S su -m root -ex
# NOTE: this script is run under root permissions
# shellcheck shell=sh disable=SC2096

# RPCS3 often needs recent Qt and Vulkan-Headers
sed -i '' 's/quarterly/latest/' /etc/pkg/FreeBSD.conf

export ASSUME_ALWAYS_YES=true
pkg info # debug

# WITH_LLVM
pkg install llvm19

# Mandatory dependencies (qt6-base is pulled via qt6-multimedia)
pkg install git ccache cmake ninja qt6-multimedia qt6-svg glew openal-soft ffmpeg

# Optional dependencies (libevdev is pulled by qt6-base)
pkg install pkgconf alsa-lib pulseaudio sdl2 evdev-proto vulkan-headers vulkan-loader
