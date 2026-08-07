#!/usr/bin/env bash
set -euo pipefail

NDK="${ANDROID_NDK_HOME:-$HOME/Android/Sdk/ndk/29.0.14206865}"
API="${ANDROID_API:-31}"
ROOT="$(cd "$(dirname "$0")" && pwd)"
SRC="${FFMPEG_SRC:-$ROOT/.work/ffmpeg-src}"
OUT="${FFMPEG_PREFIX:-$ROOT/prebuilt/ffmpeg-arm64-v8a}"
BUILD="${FFMPEG_BUILD:-$ROOT/.work/ffmpeg-build}"

TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64"
export PATH="$TOOLCHAIN/bin:$PATH"

FFMPEG_VERSION="${FFMPEG_VERSION:-8.1.1}"

if [ ! -f "$SRC/configure" ]; then
	echo "fetching ffmpeg $FFMPEG_VERSION"
	mkdir -p "$SRC"
	tmp="$(mktemp -d)"
	trap 'rm -rf "$tmp"' EXIT
	curl -fsSL "https://ffmpeg.org/releases/ffmpeg-$FFMPEG_VERSION.tar.xz" -o "$tmp/ffmpeg.tar.xz"
	tar -xJf "$tmp/ffmpeg.tar.xz" -C "$SRC" --strip-components=1
fi

mkdir -p "$BUILD"
cd "$BUILD"

"$SRC/configure" \
	--prefix="$OUT" \
	--target-os=android \
	--arch=aarch64 \
	--cpu=armv8-a \
	--enable-cross-compile \
	--cross-prefix="$TOOLCHAIN/bin/llvm-" \
	--cc="$TOOLCHAIN/bin/aarch64-linux-android$API-clang" \
	--cxx="$TOOLCHAIN/bin/aarch64-linux-android$API-clang++" \
	--ar="$TOOLCHAIN/bin/llvm-ar" \
	--nm="$TOOLCHAIN/bin/llvm-nm" \
	--ranlib="$TOOLCHAIN/bin/llvm-ranlib" \
	--strip="$TOOLCHAIN/bin/llvm-strip" \
	--sysroot="$TOOLCHAIN/sysroot" \
	--enable-static \
	--disable-shared \
	--enable-pic \
	--extra-cflags="-fPIC -fvisibility=hidden" \
	--extra-cxxflags="-fPIC -fvisibility=hidden" \
	--disable-doc \
	--disable-programs \
	--disable-avdevice \
	--disable-avfilter \
	--enable-runtime-cpudetect \
	--disable-autodetect \
	--disable-everything \
	--disable-network \
	--enable-decoder=aac --enable-decoder=aac_latm --enable-decoder=atrac3 --enable-decoder=atrac3p --enable-decoder=atrac9 \
	--enable-decoder=mp3 --enable-decoder=pcm_s16le --enable-decoder=pcm_s8 \
	--enable-decoder=h264 --enable-decoder=mpeg4 --enable-decoder=mpeg2video --enable-decoder=mjpeg --enable-decoder=mjpegb \
	--enable-encoder=pcm_s16le --enable-encoder=mp3 --enable-encoder=ac3 --enable-encoder=aac \
	--enable-encoder=ffv1 --enable-encoder=mpeg4 --enable-encoder=mjpeg \
	--enable-muxer=avi --enable-muxer=h264 --enable-muxer=mjpeg --enable-muxer=mp4 \
	--enable-demuxer=h264 --enable-demuxer=m4v --enable-demuxer=mp3 --enable-demuxer=mpegvideo --enable-demuxer=mpegps \
	--enable-demuxer=mjpeg --enable-demuxer=mov --enable-demuxer=avi --enable-demuxer=aac --enable-demuxer=pmp \
	--enable-demuxer=oma --enable-demuxer=pcm_s16le --enable-demuxer=pcm_s8 --enable-demuxer=wav \
	--enable-parser=h264 --enable-parser=mpeg4video --enable-parser=mpegaudio --enable-parser=mpegvideo \
	--enable-parser=mjpeg --enable-parser=aac --enable-parser=aac_latm \
	--enable-protocol=file \
	--enable-bsf=mjpeg2jpeg

make -j"$(nproc)"
make install

echo "FFmpeg installed to $OUT"
ls -la "$OUT/lib"
