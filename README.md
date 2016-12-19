RPCS3
=====

[![Build Status](https://travis-ci.org/RPCS3/rpcs3.svg?branch=master)](https://travis-ci.org/RPCS3/rpcs3)
[![Build status](https://ci.appveyor.com/api/projects/status/411c4clmiohtx7eo/branch/master?svg=true)](https://ci.appveyor.com/project/rpcs3/rpcs3/branch/master)
[![Coverity Status](https://img.shields.io/coverity/scan/3960.svg)](https://scan.coverity.com/projects/3960)

An open-source PlayStation 3 emulator/debugger written in C++.

You can find some basic information in the [FAQ](https://github.com/RPCS3/rpcs3/wiki/FAQ). For discussion about this emulator and PS3 emulation please visit the [official forums](http://www.emunewz.net/forum/forumdisplay.php?fid=172).

Official Discord server: https://discord.me/RPCS3

Support Nekotekina on Patreon: https://www.patreon.com/Nekotekina

### Development

If you want to contribute please take a look at the [Coding Style](https://github.com/RPCS3/rpcs3/wiki/Coding-Style), [Roadmap](https://github.com/RPCS3/rpcs3/wiki/Roadmap) and [Developer Information](https://github.com/RPCS3/rpcs3/wiki/Developer-Information) pages. You should as well contact any of the developers in the forum in order to know about the current situation of the emulator.


### Dependencies

__Windows__
* [Visual Studio 2017](https://www.visualstudio.com/downloads/)
* [Visual C++ Redistributable Packages for Visual Studio 2017](https://go.microsoft.com/fwlink/?LinkId=746572)
* [Cmake 3.1.0+](http://www.cmake.org/download/) (required; add to PATH)
* [Python 3.3+](https://www.python.org/downloads/) (required; add to PATH)

__Linux__
* GCC 5.1+ or Clang 3.5.0+ ([not GCC 6.1](https://github.com/RPCS3/rpcs3/issues/1691))
* Debian & Ubuntu: `sudo apt-get install cmake build-essential libopenal-dev libwxgtk3.0-dev libglew-dev zlib1g-dev libedit-dev libvulkan-dev git`
* Arch: `sudo pacman -S glew openal wxgtk cmake llvm`

__Mac OSX__
Mac OSX is not supported at this moment because it doesn't meet system requirements (OpenGL 4.3)
* Xcode 6+ (tested with Xcode 6.4)
* Install with Homebrew: `brew install glew wxwidgets`
* Remove '-framework QuickTime' from '_ldflags' in /usr/local/bin/wx-config

### Building

- __Windows__: </br>
1) To initialize the repository don't forget to execute `git submodule update --init` to pull the submodules. </br>
2) Open the *.SLN* file. </br>
3) Build the projects in *__BUILD_BEFORE* folder: right-click on every project > *Build*. </br>
4) Press *BUILD* > *Build Solution* or *Rebuild Solution*. </br>

- __Linux & Mac OSX__: </br>
1) `git clone https://github.com/RPCS3/rpcs3.git` </br>
2) `cd rpcs3/` </br>
3) `git submodule update --init` </br>
4) `cmake CMakeLists.txt && make GitVersion && make` </br>
5) Run RPCS3 with `./bin/rpcs3` </br>

If you are on OSX and want to build with llvm don't forget to add `-DLLVM_DIR=...` (or wherever llvm brew was installed) to cmake invocation.

When using GDB, configure it to ignore SIGSEGV signal (`handle SIGSEGV nostop noprint`).

##### CMake Build Options (Linux & Mac OSX)

- ```-DUSE_SYSTEM_LIBPNG=ON/OFF``` (default = *OFF*) </br>
Build against the shared libpng instead of using the builtin one. libpng 1.6+ highly recommended. Try this option if you get version conflict errors or only see black game icons.

- ```-DUSE_SYSTEM_FFMPEG=ON/OFF``` (default = *OFF*) </br>
Build against the shared ffmpeg libraries instead of using the builtin patched version. Try this if the builtin version breaks the OpenGL renderer for you.

### License

Most files are licensed under the terms of GNU GPLv2 License, see LICENSE file for details. Some files may be licensed differently, check appropriate file headers for details.
