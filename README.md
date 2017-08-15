RPCS3
=====

[![Build Status](https://travis-ci.org/RPCS3/rpcs3.svg?branch=master)](https://travis-ci.org/RPCS3/rpcs3)
[![Build status](https://ci.appveyor.com/api/projects/status/411c4clmiohtx7eo/branch/master?svg=true)](https://ci.appveyor.com/project/rpcs3/rpcs3/branch/master)
[![Coverity Status](https://img.shields.io/coverity/scan/3960.svg)](https://scan.coverity.com/projects/3960)

The world's first open-source PlayStation 3 emulator/debugger written in C++ for Windows and Linux.

You can find some basic information in our [**website**](https://rpcs3.net/). 
For discussion about this emulator and PS3 emulation please visit our [**forums**](http://www.emunewz.net/forum/forumdisplay.php?fid=172) and our [**Discord server**](https://discord.me/RPCS3).

[**Support Lead Developer Nekotekina on Patreon**](https://www.patreon.com/Nekotekina)


## Development

If you want to contribute please take a look at the [Coding Style](https://github.com/RPCS3/rpcs3/wiki/Coding-Style), [Roadmap](https://github.com/RPCS3/rpcs3/wiki/Roadmap) and [Developer Information](https://github.com/RPCS3/rpcs3/wiki/Developer-Information) pages. You should as well contact any of the developers in the forums or in Discord in order to know more about the current situation of the emulator.


## Dependencies

### Windows
* [Visual Studio 2015](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx)
* [Visual C++ Redistributable Packages for Visual Studio 2015](http://www.microsoft.com/en-us/download/details.aspx?id=48145)
* [Cmake 3.1.0+](https://www.cmake.org/download/) (required; add to PATH)
* [Python 3.3+](https://www.python.org/downloads/) (required; add to PATH)
* [Qt 5.7+](https://www.qt.io/download-open-source/) (required; add QTDIR `<QtInstallFolder>\5.7\msvc2015_64\` environment variable if you do not want to use the Visual Studio Qt Plugin)
* [Visual Studio Qt Plugin](https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools2015) (optional; see above)

### Linux
* [Qt 5.7+](https://www.qt.io/download-open-source/)
* GCC 5.1+ or Clang 3.5.0+ ([not GCC 6.1](https://github.com/RPCS3/rpcs3/issues/1691))
* Debian & Ubuntu: `sudo apt-get install cmake build-essential libasound2-dev libopenal-dev libglew-dev zlib1g-dev libedit-dev libvulkan-dev libudev-dev git qt5-default`
* Arch: `sudo pacman -S glew openal cmake llvm qt5-base`
* Fedora: `sudo dnf install cmake qt5-devel vulkan-devel glew glew-devel`

### Mac OSX 
Mac OSX is not supported at this moment because it doesn't meet system requirements (OpenGL 4.3)
* Xcode 6+ (tested with Xcode 6.4)
* Install with Homebrew: `brew install glew llvm qt cmake`


## Building on Windows:
To initialize the repository don't forget to execute `git submodule update --init` to pull the submodules.

### Configuring Qt

*If you're using Visual Studio 2017 without Qt plugin support (or simply dont want to use it):* 
1) Add `QTDIR` environment variable and set it to `<QtInstallFolder>\5.7\msvc2015_64\` </br>

*If you wish to use the Visual Studio plugin for Qt:* </br>
1) Go to the Qt5 menu and edit Qt5 options. Add the path to your Qt installation with compiler e.g. `C:\Qt\5.7\msvc2015_64`. </br>
2) While selecting the rpcs3qt project, go to Qt5->Project Setting and select the version you added. 

### Building the projects
1) Build the projects in *__BUILD_BEFORE* folder: right-click on every project > *Build*. </br>
2) Press *BUILD* > *Build Solution* or *Rebuild Solution*. </br>


## Building on Linux & Mac OSX:

1) `git clone https://github.com/RPCS3/rpcs3.git` </br>
2) `cd rpcs3/` </br>
3) `git submodule update --init` </br>
4) `cmake CMakeLists.txt && make GitVersion && make` </br>
5) Run RPCS3 with `./bin/rpcs3` </br>

If you are on OSX and want to build with brew llvm and qt don't forget to add the following environment variables

 * `LLVM_DIR=/usr/local/opt/llvm/` (or wherever llvm was installed).
 * `Qt5_DIR=/usr/local/opt/qt/lib/cmake/Qt5` (or wherever qt was installed).

When using GDB, configure it to ignore SIGSEGV signal (`handle SIGSEGV nostop noprint`).


## CMake Build Options (Linux & Mac OSX)

- ```-DUSE_SYSTEM_LIBPNG=ON/OFF``` (default = *OFF*) </br>
Build against the shared libpng instead of using the builtin one. libpng 1.6+ highly recommended. Try this option if you get version conflict errors or only see black game icons.

- ```-DUSE_SYSTEM_FFMPEG=ON/OFF``` (default = *OFF*) </br>
Build against the shared ffmpeg libraries instead of using the builtin patched version. Try this if the builtin version breaks the OpenGL renderer for you.


## License

Most files are licensed under the terms of GNU GPLv2 License, see LICENSE file for details. Some files may be licensed differently, check appropriate file headers for details.
