#!/usr/bin/env -S su -m root -ex
# NOTE: this script is run under root permissions
# shellcheck shell=sh disable=SC2096

# RPCS3 often needs recent Qt and Vulkan-Headers
sed -i '' 's/quarterly/latest/' /etc/pkg/FreeBSD.conf

export ASSUME_ALWAYS_YES=true
pkg info # debug

# WITH_LLVM
if pkg search -q "^llvm${LLVM_COMPILER_VER}$" >/dev/null; then
    pkg install "llvm${LLVM_COMPILER_VER}"
else
    echo "llvm${LLVM_COMPILER_VER} not found, falling back to llvm-devel"
    pkg install llvm-devel
fi

# Mandatory dependencies (qtX-base is pulled via qtX-multimedia)
pkg install git ccache cmake ninja "qt$QT_VER_MAIN-multimedia" "qt$QT_VER_MAIN-svg" glew openal-soft ffmpeg pcre2

# Optional dependencies (libevdev is pulled by qtX-base)
pkg install pkgconf alsa-lib pulseaudio sdl3 evdev-proto vulkan-headers vulkan-loader opencv
