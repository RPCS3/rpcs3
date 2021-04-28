#!/bin/sh -ex

curl -L -o "./llvm.lock" "https://github.com/RPCS3/llvm-mirror/releases/download/custom-build-win/llvmlibs_mt.7z.sha256"
curl -L -o "./glslang.lock" "https://github.com/RPCS3/glslang/releases/download/custom-build-win/glslanglibs_mt.7z.sha256"
