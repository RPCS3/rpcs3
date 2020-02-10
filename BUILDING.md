# Building

Only Windows and Linux are officially supported for building. However, various other platforms are capable of building RPCS3.
Other instructions may be found [here](https://wiki.rpcs3.net/index.php?title=Building).

## Setup your environment

### Windows 7 or later

* [CMake 3.14.1+](https://www.cmake.org/download/) (add to PATH)
* [Python 3.3+](https://www.python.org/downloads/) (add to PATH)
* [Qt 5.14+](https://www.qt.io/download-qt-installer)
* [Visual Studio 2019](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community)
* [Vulkan SDK 1.1.97.0+](https://vulkan.lunarg.com/sdk/home) (See "Install the SDK" [here](https://vulkan.lunarg.com/doc/sdk/latest/windows/getting_started.html))

**Either add the** `QTDIR` **environment variable, e.g.** `<QtInstallFolder>\5.14.1\msvc2017_64\` **, or use the [Visual Studio Qt Plugin](https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools-19123)**

### Linux

These are the essentials tools to build RPCS3 on Linux. Some of them can be installed through your favorite package manager.

* Clang 5.0+ or GCC 8.1+
* [CMake 3.8.2+](https://www.cmake.org/download/)
* [Qt 5.14+](https://www.qt.io/download-qt-installer)
* [Vulkan SDK 1.1.97.0+](https://vulkan.lunarg.com/sdk/home) (See "Install the SDK" [here](https://vulkan.lunarg.com/doc/sdk/latest/linux/getting_started.html))
* [SDL2](https://www.libsdl.org/download-2.0.php) (for the FAudio backend)

**If you have an NVIDIA GPU, you may need to install the libglvnd package.**

#### Arch Linux

    sudo pacman -S glew openal cmake vulkan-validation-layers qt5-base qt5-declarative sdl2

#### Debian & Ubuntu

    sudo apt-get install cmake build-essential libasound2-dev libpulse-dev libopenal-dev libglew-dev zlib1g-dev libedit-dev libvulkan-dev libudev-dev git qt5-default libevdev-dev qtdeclarative5-dev qtbase5-private-dev libsdl2-2.0 libsdl2-dev

##### GCC 8.x installation

If the `gcc-8` package is not available on your system, use the following command
```
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
```
Then
```
sudo apt-get update
sudo apt-get install gcc-8 g++-8
```

You can either use `update-alternatives` to setup `gcc-8`/`g++-8` as your default compilers or prefix any `cmake` command by `CXX=g++-8 CC=gcc-8 ` to use it.

##### Vulkan SDK

For Ubuntu systems, it is strongly recommended to use the PPA from [LunarG](https://packages.lunarg.com/) which will provide a compatible Vulkan SDK to compile RPCS3. If your Vulkan SDK is older, it can lead to compilation errors.

Ubuntu 18.04 (Bionic Beaver)
```
wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.1.106-bionic.list http://packages.lunarg.com/vulkan/1.1.106/lunarg-vulkan-1.1.106-bionic.list
sudo apt update
sudo apt install vulkan-sdk
```

Ubuntu 16.04 (Xenial Xerus)
```
wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.1.106-xenial.list http://packages.lunarg.com/vulkan/1.1.106/lunarg-vulkan-1.1.106-xenial.list
sudo apt update
sudo apt install vulkan-sdk
```

#### Fedora

    sudo dnf install alsa-lib-devel cmake glew glew-devel libatomic libevdev-devel libudev-devel openal-devel qt5-devel vulkan-devel

#### OpenSUSE

    sudo zypper install git cmake libasound2 libpulse-devel openal-soft-devel glew-devel zlib-devel libedit-devel vulkan-devel libudev-devel libqt5-qtbase-devel libevdev-devel

## Setup the project

Clone and initialize the repository

```
git clone https://github.com/RPCS3/rpcs3.git
cd rpcs3
git submodule update --init
```

### Windows

#### Configuring the Qt plugin (if used)

1) Go to the Qt5 menu and edit Qt5 options.
2) Add the path to your Qt installation with compiler e.g. `<QtInstallFolder>\5.14.1\msvc2017_64`.
3) While selecting the rpcs3qt project, go to Qt5->Project Setting and select the version you added.

#### Building the projects

Open `rpcs3.sln`. The recommended build configuration is `Release - LLVM` for all purposes.

You may want to download precompiled [LLVM libs](https://github.com/RPCS3/llvm/releases/download/continuous-master/llvmlibs.7z) and extract to root rpcs3 folder (which contains `rpcs3.sln`), as well as download and extract [additional libs](https://drive.google.com/uc?export=download&id=1A2eOMmCO714i0U7J0qI4aEMKnuWl8l_R) to `lib\%CONFIGURATION%-x64\` to speed up compilation time (unoptimised/debug libs are currently not available precompiled).

If you're not using precompiled libs, build the projects in *__BUILD_BEFORE* folder: right-click on every project > *Build*.

`Build > Build Solution`

### Linux

While still in the project root:

1) `cd .. && mkdir rpcs3_build && cd rpcs3_build`
2) `cmake ../rpcs3/ && make` or `CXX=g++-8 CC=gcc-8 cmake ../rpcs3/ && make` to force these compilers
3) Run RPCS3 with `./bin/rpcs3`

When using GDB, configure it to ignore SIGSEGV signal (`handle SIGSEGV nostop noprint`).
If desired, use the various build options in [CMakeLists](https://github.com/RPCS3/rpcs3/blob/master/CMakeLists.txt).
