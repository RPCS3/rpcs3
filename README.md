RPCS3
=====

[![Build Status](https://travis-ci.org/RPCS3/rpcs3.svg?branch=master)](https://travis-ci.org/RPCS3/rpcs3)
[![Build status](https://ci.appveyor.com/api/projects/status/411c4clmiohtx7eo/branch/master?svg=true)](https://ci.appveyor.com/project/rpcs3/rpcs3/branch/master)

The world's first free and open-source PlayStation 3 emulator/debugger, written in C++ for Windows and Linux.

You can find some basic information on our [**website**](https://rpcs3.net/). Game info is being populated on the [**Wiki**](https://wiki.rpcs3.net/).
For discussion about this emulator, PS3 emulation, and game compatibility reports, please visit our [**forums**](https://forums.rpcs3.net) and our [**Discord server**](https://discord.me/RPCS3).

[**Support Lead Developers Nekotekina and kd-11 on Patreon**](https://www.patreon.com/Nekotekina)


## Contributing

If you want to help the project but do not code, the best way to help out is to test games and make bug reports. See:
* [Quickstart](https://rpcs3.net/quickstart)

If you want to contribute as a developer, please take a look at the following pages:

* [Coding Style](https://github.com/RPCS3/rpcs3/wiki/Coding-Style)
* [Developer Information](https://github.com/RPCS3/rpcs3/wiki/Developer-Information)
* [Roadmap](https://rpcs3.net/roadmap)

You should also contact any of the developers in the forums or in the Discord server to learn more about the current state of the emulator.


## Dependencies

### Windows
* [Visual Studio 2017](https://www.visualstudio.com/en/downloads/)
* [Visual C++ Redistributable Packages for Visual Studio 2017](https://go.microsoft.com/fwlink/?LinkId=746572)
* [CMake 3.8.2+](https://www.cmake.org/download/) (add to PATH)
* [Python 3.3+](https://www.python.org/downloads/) (add to PATH)
* [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) (See "Install the SDK" [here](https://vulkan.lunarg.com/doc/sdk/latest/windows/getting_started.html))
* [Qt 5.10+](https://www.qt.io/download-qt-installer) (Avoid 5.11.1, due to a bug)


**Either add the** `QTDIR` **environment variable, e.g.** `<QtInstallFolder>\5.11.2\msvc2017_64\` **, or use the [Visual Studio Qt Plugin](https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools-19123)**

### Linux
* [Qt 5.10+](https://www.qt.io/download-open-source/) (Avoid 5.11.1, due to a bug)
* GCC 7.3+ or Clang 5.0+
* CMake 3.8.2+
* Debian & Ubuntu: `sudo apt-get install cmake build-essential libasound2-dev libpulse-dev libopenal-dev libglew-dev zlib1g-dev libedit-dev libvulkan-dev libudev-dev git qt5-default libevdev-dev qtdeclarative5-dev`
* Arch: `sudo pacman -S glew openal cmake vulkan-validation-layers qt5-base qt5-declarative`
* Fedora: `sudo dnf install alsa-lib-devel cmake glew glew-devel libatomic libevdev-devel libudev-devel openal-devel qt5-devel vulkan-devel`
* OpenSUSE: `sudo zypper install git cmake libasound2 libpulse-devel openal-soft-devel glew-devel zlib-devel libedit-devel vulkan-devel libudev-devel libqt5-qtbase-devel libevdev-devel`

**If you have an NVIDIA GPU, you may need to install the libglvnd package.**


## Building

Only Windows and Linux are officially supported for building. However, various other platforms are capable of building RPCS3. Other instructions may be found [here](https://wiki.rpcs3.net/index.php?title=Building).

Clone and initialize the repository:

1) `git clone https://github.com/RPCS3/rpcs3.git`
2) `cd rpcs3/`
3) `git submodule update --init`

### Windows
#### Configuring the Qt plugin (if used)

1) Go to the Qt5 menu and edit Qt5 options.
2) Add the path to your Qt installation with compiler e.g. `<QtInstallFolder>\5.11.2\msvc2017_64`.
3) While selecting the rpcs3qt project, go to Qt5->Project Setting and select the version you added.

#### Building the projects

Open `rpcs3.sln`. The recommended build configuration is `Release - LLVM` for all purposes.

You may want to download precompiled [LLVM libs](https://github.com/RPCS3/llvm/releases/download/continuous-master/llvmlibs.7z) and extract to root rpcs3 folder (which contains `rpcs3.sln`), as well as download and extract [additional libs](https://drive.google.com/uc?export=download&id=1A2eOMmCO714i0U7J0qI4aEMKnuWl8l_R) to `lib\%CONFIGURATION%-x64\` to speed up compilation time (unoptimised/debug libs are currently not available precompiled).

If you're not using precompiled libs, build the projects in *__BUILD_BEFORE* folder: right-click on every project > *Build*.


`Build > Build Solution`


### Linux

While still in the project root:

1) `cd .. && mkdir rpcs3_build && cd rpcs3_build`
2) `cmake ../rpcs3/ && make`
3) Run RPCS3 with `./bin/rpcs3`

When using GDB, configure it to ignore SIGSEGV signal (`handle SIGSEGV nostop noprint`).
If desired, use the various build options in [CMakeLists](https://github.com/RPCS3/rpcs3/blob/master/CMakeLists.txt).

## License

Most files are licensed under the terms of GNU GPLv2 License; see LICENSE file for details. Some files may be licensed differently; check appropriate file headers for details.
