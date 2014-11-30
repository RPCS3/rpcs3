REM You need cmake and python to update the project files
REM this script relies on CWD being the path that this script is in

cmake -G "Visual Studio 12 Win64" -DCMAKE_CONFIGURATION_TYPES="Debug;Release" -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_BUILD_RUNTIME=OFF -DLLVM_BUILD_TOOLS=OFF -DLLVM_INCLUDE_DOCS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_TOOLS=OFF -DLLVM_INCLUDE_UTILS=OFF -DWITH_POLLY=OFF ../llvm

RD /S /Q cmake
RD /S /Q CMakeFiles
RD /S /Q projects
RD /S /Q share
RD /S /Q tools
DEL ALL_BUILD.vcxproj
DEL ALL_BUILD.vcxproj.filters
DEL CMakeCache.txt
DEL cmake_install.cmake
DEL CPackConfig.cmake
DEL CPackSourceConfig.cmake
DEL DummyConfigureOutput
DEL INSTALL.vcxproj
DEL INSTALL.vcxproj.filters
DEL LLVM.sdf
DEL LLVM.sln
DEL LLVMBuild.cmake
DEL PACKAGE.vcxproj
DEL PACKAGE.vcxproj.filters
DEL ZERO_CHECK.vcxproj
DEL ZERO_CHECK.vcxproj.filters
DEL include\llvm\llvm_headers_do_not_build.vcxproj
DEL include\llvm\llvm_headers_do_not_build.vcxproj.filters
python make_paths_relative.py

