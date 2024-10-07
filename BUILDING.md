# Building

Only Windows and Linux are officially supported for building. However, various other platforms are capable of building RPCS3.
Other instructions may be found [here](https://wiki.rpcs3.net/index.php?title=Building).

## Setup your environment

### Windows 10 or later

The following tools are required to build RPCS3 on Windows 10 or later:
- [Visual Studio 2022](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community) (or at least Visual Studio 2019 16.11.xx+ as C++20 is not included in previous versions)
- **Optional** - [CMake 3.28.0+](https://www.cmake.org/download/) (add to PATH)

  **NOTES:**
  - **Visual Studio 2022** integrates **CMake 3.29+** and it also supports both the `sln` solution (`.sln`, `.vcxproj`) and `CMake` solution (`CMakeLists.txt`, `CMakePresets.json`).
    See sections [Building with Visual Studio sln solution](#building-with-visual-studio-sln-solution) and [Building with Visual Studio CMake solution](#building-with-visual-studio-cmake-solution)
    on how to build the project with **Visual Studio**.
  - Install and use this standalone **CMake** tool just in case of your preference. See section [Building with standalone CMake tool](#building-with-standalone-cmake-tool) on how to build the project
    with standalone **CMake** tool.

- [Python 3.6+](https://www.python.org/downloads/) (add to PATH)
- [Qt 6.7.3](https://www.qt.io/download-qt-installer)
- [Vulkan SDK 1.3.268.0](https://vulkan.lunarg.com/sdk/home) (see "Install the SDK" [here](https://vulkan.lunarg.com/doc/sdk/latest/windows/getting_started.html)) for now future SDKs don't work. You need precisely 1.3.268.0.

The `sln` solution available only on **Visual Studio** is the preferred building solution. It allows to build the **RPCS3** application in `Release` and `Debug` mode
while the `CMake` solution is currently limited to `Debug` mode only.

In order to build **RPCS3** with the `sln` solution (with **Visual Studio**), **Qt** libs need to be detected. To detect the libs:
- add and set the `QTDIR` environment variable, e.g. `<QtInstallFolder>\6.7.3\msvc2019_64\`
- or use the [Visual Studio Qt Plugin](https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools2019)

  **NOTE:** If you have issues with the **Visual Studio Qt Plugin**, you may want to uninstall it and install the [Legacy Qt Plugin](https://marketplace.visualstudio.com/items?itemName=TheQtCompany.LEGACYQtVisualStudioTools2019) instead.

In order to build **RPCS3** with the `CMake` solution (with both **Visual Studio** and standalone **CMake** tool):
- add and set the `CMAKE_PREFIX_PATH` environment variable to the **Qt** libs path, e.g. `<QtInstallFolder>\6.7.3\msvc2019_64\`

### Linux

These are the essentials tools to build RPCS3 on Linux. Some of them can be installed through your favorite package manager:
- Clang 17+ or GCC 13+
- [CMake 3.28.0+](https://www.cmake.org/download/)
- [Qt 6.7.3](https://www.qt.io/download-qt-installer)
- [Vulkan SDK 1.3.268.0](https://vulkan.lunarg.com/sdk/home) (See "Install the SDK" [here](https://vulkan.lunarg.com/doc/sdk/latest/linux/getting_started.html)) for now future SDKs don't work. You need precisely 1.3.268.0.
- [SDL2](https://github.com/libsdl-org/SDL/releases) (for the FAudio backend)

**If you have an NVIDIA GPU, you may need to install the libglvnd package.**

#### Arch Linux

    sudo pacman -S glew openal cmake vulkan-validation-layers qt6-base qt6-declarative qt6-multimedia qt6-svg sdl2 sndio jack2 base-devel

#### Debian & Ubuntu

    sudo apt-get install build-essential libasound2-dev libpulse-dev libopenal-dev libglew-dev zlib1g-dev libedit-dev libvulkan-dev libudev-dev git libevdev-dev libsdl2-2.0 libsdl2-dev libjack-dev libsndio-dev

Ubuntu is usually horrendously out of date, and some packages need to be downloaded by hand. This part is for Qt, GCC, Vulkan, and CMake

##### Qt PPA

Ubuntu usually does not have a new enough Qt package to suit rpcs3's needs. There is currently no PPA available to work around this.

##### GCC 13.x installation

If the `gcc-13` package is not available on your system, use the following commands
```
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-13 g++-13
```

You can either use `update-alternatives` to setup `gcc-13`/`g++-13` as your default compilers or prefix any `cmake` command by `CXX=g++-13 CC=gcc-13 ` to use it.

##### Vulkan SDK

For Ubuntu systems, it is strongly recommended to use the PPA from [LunarG](https://packages.lunarg.com/) which will provide a compatible Vulkan SDK to compile RPCS3. If your Vulkan SDK is older, it can lead to compilation errors.
```
. /etc/os-release
wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.3.268-$UBUNTU_CODENAME.list https://packages.lunarg.com/vulkan/1.3.268/lunarg-vulkan-1.3.268-$UBUNTU_CODENAME.list
sudo apt update
sudo apt install vulkan-sdk
```

##### CMake

```
. /etc/os-release
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ $UBUNTU_CODENAME main"
sudo apt-get update
sudo apt-get install kitware-archive-keyring
sudo apt-key --keyring /etc/apt/trusted.gpg del C1F34CDD40CD72DA
sudo apt-get install cmake
```

#### Fedora

    sudo dnf install alsa-lib-devel cmake glew glew-devel libatomic libevdev-devel libudev-devel openal-devel qt6-qtbase-devel qt6-qtbase-private-devel vulkan-devel pipewire-jack-audio-connection-kit-devel qt6-qtmultimedia-devel qt6-qtsvg-devel

#### OpenSUSE

    sudo zypper install git cmake libasound2 libpulse-devel openal-soft-devel glew-devel zlib-devel libedit-devel vulkan-devel libudev-devel libqt6-qtbase-devel libqt6-qtmultimedia-devel libqt6-qtsvg-devel libQt6Gui-private-headers-devel libevdev-devel libsndio7_1 libjack-devel

## Setup the project

Clone and initialize the repository

```
git clone https://github.com/RPCS3/rpcs3.git
cd rpcs3
git submodule update --init
```

### Windows

#### Building with Visual Studio sln solution

Start **Visual Studio**, click on `Open a project or solution` and select the `rpcs3.sln` file inside the RPCS3's root folder

##### Configuring the Qt Plugin (if used)

1) go to `Extensions->Qt VS Tools->Qt Versions`
2) add the path to your Qt installation with compiler e.g. `<QtInstallFolder>\6.7.3\msvc2019_64`, version will fill in automatically
3) go to `Extensions->Qt VS Tools->Options->Legacy Project Format`. (Only available in the **Legacy Qt Plugin**)
4) set `Build: Run pre-build setup` to `true`. (Only available in the **Legacy Qt Plugin**)

##### Building the projects

**NOTE:** The recommended build configuration is `Release`. (On older revisions: `Release - LLVM`)

You may want to download the precompiled [LLVM libs](https://github.com/RPCS3/llvm-mirror/releases/download/custom-build-win-16.0.1/llvmlibs_mt.7z) and extract them to `3rdparty\llvm\`,
as well as download and extract the [additional libs](https://github.com/RPCS3/glslang/releases/latest/download/glslanglibs_mt.7z) to `lib\%CONFIGURATION%-x64\` to speed up compilation
time (unoptimised/debug libs are currently not available precompiled).

If you're not using the precompiled libs, build the following projects in `__BUILD_BEFORE` folder by right-clicking on a project and then click on `Build`:
- `glslang`
- either `llvm_build`
- or `llvm_build_clang_cl`

Afterwards:

`Build > Build Solution`

#### Building with Visual Studio CMake solution

Start **Visual Studio**, click on `Open a local folder` and select the RPCS3's root folder

Once the project is open on VS, from the `Solution Explorer` panel:
1) right-click on `rpcs3` and then click on `Switch to CMake Targets View`
2) from the `Configuration` drop-down menu select `Windows x64`
3) right-click on `CMakeLists.txt Project` and then click on `Configure Cache`
4) once the cache is created, the `rpcs3 project` will be available
5) right-click on `rpcs3 Project` and then click on `Build All`
6) once the build is completed, the **RPCS3** application will be available under the `<rpcs3_root>\build-msvc\bin` folder

#### Building with standalone CMake tool

In case you preferred to install and use the standalone **CMake** tool:
1) move on the RPCS3's root folder
2) execute the following commands to create the cache and to build the application, respectively:
  ```
  cmake --preset msvc
  cmake --build build-msvc
  ```
3) once the build is completed, the **RPCS3** application will be available under the `<rpcs3_root>\build-msvc\bin` folder

### Linux

While still in the project root:

1) `cd .. && mkdir --parents rpcs3_build && cd rpcs3_build`
2) `cmake ../rpcs3/ && make` or `CXX=g++-13 CC=gcc-13 cmake ../rpcs3/ && make` to force these compilers
3) run RPCS3 with `./bin/rpcs3`

If compiling for ARM, pass the flag `-DUSE_NATIVE_INSTRUCTIONS=OFF` to the cmake command. This resolves some Neon errors when compiling our SIMD headers.

When using GDB, configure it to ignore SIGSEGV signal (`handle SIGSEGV nostop noprint`).
If desired, use the various build options in [CMakeLists](https://github.com/RPCS3/rpcs3/blob/master/CMakeLists.txt).
