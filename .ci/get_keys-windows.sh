#!/bin/sh -ex

curl -fLo "./llvm.lock" "https://github.com/RPCS3/llvm-mirror/releases/download/custom-build-win-19.1.7/llvmlibs_mt.7z.sha256"
curl -fLo "./glslang.lock" "https://github.com/RPCS3/glslang/releases/download/custom-build-win/glslanglibs_mt.7z.sha256"
