#!/bin/sh -ex

curl -fLo "./llvm.lock" "https://github.com/RPCS3/llvm-mirror/releases/download/custom-build-win-${LLVM_VER}/llvmlibs_mt.7z.sha256"
