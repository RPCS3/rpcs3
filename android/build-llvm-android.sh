#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
NDK="${ANDROID_NDK_HOME:-$HOME/Android/Sdk/ndk/29.0.14206865}"
API="${ANDROID_API:-31}"
SRC="${LLVM_SRC:-$ROOT/../3rdparty/llvm/llvm/llvm}"
HOST_TOOLS="${LLVM_HOST_TOOLS:-$ROOT/.work/llvm-host/bin}"
BUILD="${LLVM_BUILD:-$ROOT/.work/llvm-android}"
OUT="${LLVM_PREFIX:-$ROOT/prebuilt/llvm-arm64-v8a}"

if [ ! -x "$HOST_TOOLS/llvm-tblgen" ]; then
	echo "building host tablegen"
	HOST_BUILD="${LLVM_HOST_BUILD:-$ROOT/.work/llvm-host-build}"
	cmake -G Ninja -S "$SRC" -B "$HOST_BUILD" \
		-DCMAKE_BUILD_TYPE=Release \
		-DLLVM_TARGETS_TO_BUILD=AArch64 \
		-DLLVM_ENABLE_PROJECTS="" \
		-DLLVM_ENABLE_RUNTIMES="" \
		-DLLVM_INCLUDE_TESTS=OFF \
		-DLLVM_INCLUDE_EXAMPLES=OFF \
		-DLLVM_INCLUDE_BENCHMARKS=OFF \
		-DLLVM_INCLUDE_DOCS=OFF
	cmake --build "$HOST_BUILD" --target llvm-tblgen llvm-min-tblgen
	mkdir -p "$HOST_TOOLS"
	for tool in llvm-tblgen llvm-min-tblgen; do
		if [ -x "$HOST_BUILD/bin/$tool" ]; then
			cp "$HOST_BUILD/bin/$tool" "$HOST_TOOLS/"
		fi
	done
fi

if [ ! -x "$HOST_TOOLS/llvm-tblgen" ]; then
	echo "host tablegen still missing at $HOST_TOOLS" >&2
	exit 1
fi

cmake -G Ninja -S "$SRC" -B "$BUILD" \
	-DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
	-DANDROID_ABI=arm64-v8a \
	-DANDROID_PLATFORM="android-$API" \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX="$OUT" \
	-DLLVM_TARGETS_TO_BUILD=AArch64 \
	-DLLVM_TARGET_ARCH=AArch64 \
	-DLLVM_HOST_TRIPLE="aarch64-unknown-linux-android$API" \
	-DLLVM_DEFAULT_TARGET_TRIPLE="aarch64-unknown-linux-android$API" \
	-DLLVM_NATIVE_TOOL_DIR="$HOST_TOOLS" \
	-DLLVM_TABLEGEN="$HOST_TOOLS/llvm-tblgen" \
	-DLLVM_ENABLE_PROJECTS="" \
	-DLLVM_ENABLE_RUNTIMES="" \
	-DLLVM_BUILD_TOOLS=OFF \
	-DLLVM_INCLUDE_TOOLS=OFF \
	-DLLVM_BUILD_UTILS=OFF \
	-DLLVM_INCLUDE_UTILS=OFF \
	-DLLVM_BUILD_TESTS=OFF \
	-DLLVM_INCLUDE_TESTS=OFF \
	-DLLVM_INCLUDE_EXAMPLES=OFF \
	-DLLVM_INCLUDE_BENCHMARKS=OFF \
	-DLLVM_INCLUDE_DOCS=OFF \
	-DLLVM_BUILD_RUNTIME=OFF \
	-DLLVM_ENABLE_ZLIB=OFF \
	-DLLVM_ENABLE_ZSTD=OFF \
	-DLLVM_ENABLE_LIBXML2=OFF \
	-DLLVM_ENABLE_TERMINFO=OFF \
	-DLLVM_ENABLE_LIBEDIT=OFF \
	-DLLVM_ENABLE_THREADS=ON \
	-DLLVM_ENABLE_PIC=ON \
	-DLLVM_ENABLE_ASSERTIONS=OFF \
	-DLLVM_ENABLE_UNWIND_TABLES=OFF \
	-DLLVM_USE_PERF=OFF

ninja -C "$BUILD" -j"$(nproc)"
ninja -C "$BUILD" install

echo "LLVM installed to $OUT"
ls "$OUT/lib" | head
