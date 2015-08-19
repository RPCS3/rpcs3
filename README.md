RPCS3
=====

[![Build Status](https://travis-ci.org/RPCS3/rpcs3.svg?branch=master)](https://travis-ci.org/RPCS3/rpcs3)
[![Build status](https://ci.appveyor.com/api/projects/status/411c4clmiohtx7eo/branch/master?svg=true)](https://ci.appveyor.com/project/rpcs3/rpcs3/branch/master)
[![Coverity Status](https://scan.coverity.com/projects/3960/badge.svg)](https://scan.coverity.com/projects/3960)
[![Coverage Status](https://coveralls.io/repos/DHrpcs3/rpcs3/badge.svg)](https://coveralls.io/r/DHrpcs3/rpcs3)

An open-source PlayStation 3 emulator/debugger written in C++.

You can find some basic information in the [FAQ](https://github.com/DHrpcs3/rpcs3/wiki/FAQ). For discussion about this emulator and PS3 emulation please visit the [official forums](http://www.emunewz.net/forum/forumdisplay.php?fid=162).


### Development

If you want to contribute please take a took at the [Coding Style](https://github.com/DHrpcs3/rpcs3/wiki/Coding-Style), [Roadmap](https://github.com/DHrpcs3/rpcs3/wiki/Roadmap) and [Developer Information](https://github.com/DHrpcs3/rpcs3/wiki/Developer-Information) pages. You should as well contact any of the developers in the forum in order to know about the current situation of the emulator.


### Dependencies

__Windows__
* [Visual C++ Redistributable Packages for Visual Studio 2015](http://www.microsoft.com/en-us/download/details.aspx?id=48145)

__Linux__
* Debian & Ubuntu: `sudo apt-get install libopenal-dev libwxgtk3.0-dev build-essential libglew-dev`
* Arch: `sudo pacman -S glew openal wxgtk cmake llvm`

__Mac OSX__
* Install with Homebrew: `brew install glew wxwidgets` (add `llvm36` to that list if you want to build with ppu jit)
* Remove '-framework QuickTime' from '_ldflags' in /usr/local/bin/wx-config


### Building

To initialize the repository don't forget to execute `git submodule update --init` to pull the wxWidgets source.
* __Windows__: Install *Visual Studio 2015*, *Cmake 3.1.0+*, *Python* and select *Add in the PATH variable* for both. Then open the *.SLN* file, and press *Build* > *Rebuild Solution*.
* __Linux & Mac OSX__:
`cd rpcs3 && cmake CMakeLists.txt && make && cd ../` Then run with `cd bin && ./rpcs3`
If you are on OSX and want to build with llvm don't forget to add `-DLLVM_DIR=/usr/local/opt/llvm36/lib/llvm-3.6/share/llvm/cmake` (or wherever llvm brew was installed) to cmake invocation.

### Support
* [Donate by PayPal](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=MPJ3S9XQXCE3G)
