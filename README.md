RPCS3
=====

[![Build Status](https://travis-ci.org/RPCS3/rpcs3.svg?branch=master)](https://travis-ci.org/RPCS3/rpcs3)
[![Build status](https://ci.appveyor.com/api/projects/status/411c4clmiohtx7eo/branch/master?svg=true)](https://ci.appveyor.com/project/rpcs3/rpcs3/branch/master)

The world's first open-source PlayStation 3 emulator/debugger, written in C++ for Windows and Linux.

You can find some basic information on our [**website**](https://rpcs3.net/). Game info is being populated on the [**Wiki**](https://wiki.rpcs3.net/).
For discussion about this emulator, PS3 emulation, and game compatibility reports, please visit our [**forums**](https://forums.rpcs3.net) and our [**Discord server**](https://discord.me/RPCS3).

[**Support Lead Developers Nekotekina and kd-11 on Patreon**](https://www.patreon.com/Nekotekina)


## Development

If you want to contribute, please take a look at the [Coding Style](https://github.com/RPCS3/rpcs3/wiki/Coding-Style), [Roadmap](https://github.com/RPCS3/rpcs3/wiki/Roadmap) and [Developer Information](https://github.com/RPCS3/rpcs3/wiki/Developer-Information) pages. You should also contact any of the developers in the forums or in the Discord server to learn more about the current state of the emulator.


## Dependencies

### Windows
* [Visual Studio 2017](https://www.visualstudio.com/en/downloads/)
* [Visual C++ Redistributable Packages for Visual Studio 2017](https://go.microsoft.com/fwlink/?LinkId=746572)
* [CMake 3.8.2+](https://www.cmake.org/download/) (add to PATH)
* [Python 3.3+](https://www.python.org/downloads/) (add to PATH)
* [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) (See "Install the SDK" [here](https://vulkan.lunarg.com/doc/sdk/latest/windows/getting_started.html))
* [Qt 5.10+](https://www.qt.io/download-open-source/)


**Either add the** `QTDIR` **environment variable, e.g.** `<QtInstallFolder>\5.11.1\msvc2017_64\` **, or use the [Visual Studio Qt Plugin](https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools2017)**

### Linux
* [Qt 5.10+](https://www.qt.io/download-open-source/)
* GCC 7.3+ or Clang 5.0+
* CMake 3.8.2+
* Debian & Ubuntu: `sudo apt-get install cmake build-essential libasound2-dev libpulse-dev libopenal-dev libglew-dev zlib1g-dev libedit-dev libvulkan-dev libudev-dev git qt5-default`
* Arch: `sudo pacman -S glew openal cmake vulkan-validation-layers qt5-base`
* Fedora: `sudo dnf install alsa-lib-devel cmake glew glew-devel libatomic libevdev-devel libudev-devel openal-devel qt5-devel vulkan-devel`
* OpenSUSE: `sudo zypper install git cmake libasound2 libpulse-devel openal-soft-devel glew-devel zlib-devel libedit-devel vulkan-devel libudev-devel libqt5-qtbase-devel libevdev-devel`

**If you have an NVIDIA GPU, you may need to install the libglvnd package.**

### MacOS
MacOS is not supported at this moment because it doesn't meet system requirements (OpenGL 4.3)
* Xcode 10
* Install with Homebrew: `brew install glew llvm qt cmake`


## Building on Windows:
To initialize the repository, don't forget to execute `git submodule update --init` to pull the submodules.

### Configuring the Qt plugin (if used)

1) Go to the Qt5 menu and edit Qt5 options. Add the path to your Qt installation with compiler e.g. `<QtInstallFolder>\5.11.1\msvc2017_64`.
2) While selecting the rpcs3qt project, go to Qt5->Project Setting and select the version you added.

### Building the projects

Open `rpcs3.sln`. The recommended build configuration is `Release - LLVM` for all purposes.

You may want to download precompiled [LLVM libs](https://github.com/RPCS3/llvm/releases/download/continuous-master/llvmlibs.7z) and extract to root rpcs3 folder (which contains `rpcs3.sln`), as well as download and extract [additional libs](https://drive.google.com/uc?export=download&id=1A2eOMmCO714i0U7J0qI4aEMKnuWl8l_R) to `lib\%CONFIGURATION%-x64\` to speed up compilation time (unoptimised/debug libs are currently not available precompiled).

If you're not using precompiled libs, build the projects in *__BUILD_BEFORE* folder: right-click on every project > *Build*.


`Build > Build Solution`


## Building on Windows (MinGW):

1) Install packages
- `pacman -S base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-yasm mingw-w64-x86_64-python2 mingw-w64-x86_64-ntldd-git mingw-w64-x86_64-qt5 mingw-w64-x86_64-openal mingw-w64-x86_64-glew git`
2) Clone repository
- `git clone https://github.com/RPCS3/rpcs3.git`
3) Update submodules
- `cd rpcs3`
- `git submodule update --init`
- `cd ..`
4) Configure and compile RPCS3
- `mkdir rpcs3_build && cd rpcs3_build`
- `cmake -G "MSYS Makefiles" -DCMAKE_MAKE_PROGRAM=mingw32-make ../rpcs3/`
- `mingw32-make.exe GitVersion && mingw32-make.exe discord-rpc`
- If you use ```-DUSE_SYSTEM_FFMPEG=OFF```, run `mingw32-make ffmpeg-mingw`
5) Build RPCS3
- Run `mingw32-make` or `mingw32-make -jX` where X is your CPU cores.
6) Copy dependencies
- `cd ./bin`
- `for l in $(ntldd.exe -R rpcs3.exe|grep mingw64|sed -e 's/^[ \t]*//'|cut -d' ' -f3);do cp $l .;done`
7) Copy qt plugins
- `mkdir -p ./qt/plugins/{bearer,imageformats,platforms,styles}`
- `cp /mingw64/share/qt5/plugins/bearer/qgenericbearer.dll ./qt/plugins/bearer/`
- `cp /mingw64/share/qt5/plugins/imageformats/{qgif.dll,qicns.dll,qico.dll,qjpeg.dll,qtga.dll,qtiff.dll,qwbmp.dll,qwebp.dll} ./qt/plugins/imageformats/`
- `cp /mingw64/share/qt5/plugins/platforms/qwindows.dll ./qt/plugins/platforms/`
- `cp /mingw64/share/qt5/plugins/styles/qwindowsvistastyle.dll ./qt/plugins/styles/`
8) Run RPCS3 with `./rpcs3`


## Building on Linux & Mac OS:

1) `git clone https://github.com/RPCS3/rpcs3.git`
2) `cd rpcs3/`
3) `git submodule update --init`
4) `cd ../ && mkdir rpcs3_build && cd rpcs3_build`
4) `cmake ../rpcs3/ && make GitVersion && make`
5) Run RPCS3 with `./bin/rpcs3`

If you are on MacOS and want to build with brew llvm and qt, don't forget to add the following environment variables:

 * `LLVM_DIR=/usr/local/opt/llvm/` (or wherever llvm was installed).
 * `Qt5_DIR=/usr/local/opt/qt/lib/cmake/Qt5` (or wherever qt was installed).

When using GDB, configure it to ignore SIGSEGV signal (`handle SIGSEGV nostop noprint`).


## CMake Build Options (Linux & Mac OS)

- ```-DUSE_SYSTEM_LIBPNG=ON/OFF``` (default = *OFF*)
Build against the shared libpng instead of using the built-in one. libpng 1.6+ highly recommended. Try this option if you get version conflict errors or only see black game icons.

- ```-DUSE_SYSTEM_FFMPEG=ON/OFF``` (default = *OFF*)
Build against the shared ffmpeg libraries instead of using the built-in patched version. Try this if the built-in version breaks the OpenGL renderer for you.

- ```-DWITHOUT_LLVM=ON/OFF``` (default = *OFF*)
This forces RPCS3 to build without LLVM (not recommended).

- ```-DWITH_GDB=ON/OFF``` (default = *OFF*)
This builds RPCS3 with support for debugging PS3 games using gdb.

- ```-DUSE_VULKAN=ON/OFF``` (default = *ON*)
This builds RPCS3 with Vulkan support.

- ```-DUSE_NATIVE_INSTRUCTIONS=ON/OFF``` (default = *ON*)
This builds RPCS3 with -march=native, which is useful for local builds, but not good for packages.

## License

Most files are licensed under the terms of GNU GPLv2 License; see LICENSE file for details. Some files may be licensed differently; check appropriate file headers for details.
