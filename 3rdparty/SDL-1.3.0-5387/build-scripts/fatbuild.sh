#!/bin/sh
#
# Build a fat binary on Mac OS X, thanks Ryan!

# Number of CPUs (for make -j)
NCPU=`sysctl -n hw.ncpu`
if test x$NJOB = x; then
    NJOB=$NCPU
fi

# SDK path
if test x$SDK_PATH = x; then
    SDK_PATH=/Developer/SDKs
fi

# Generic, cross-platform CFLAGS you always want go here.
CFLAGS="-O3 -g -pipe"

# They changed this from "darwin9" to "darwin10" in Xcode 3.2 (Snow Leopard).
GCCUSRPATH_PPC=`ls -d $SDK_PATH/MacOSX10.4u.sdk/usr/lib/gcc/powerpc-apple-darwin*/4.0.1`
if [ ! -d "$GCCUSRPATH_PPC" ]; then
    echo "Couldn't find any GCC usr path for 32-bit ppc"
    exit 1
fi
GCCUSRPATH_PPC64=`ls -d $SDK_PATH/MacOSX10.5.sdk/usr/lib/gcc/powerpc-apple-darwin*/4.0.1`
if [ ! -d "$GCCUSRPATH_PPC64" ]; then
    echo "Couldn't find any GCC usr path for 64-bit ppc"
    exit 1
fi

# PowerPC 32-bit configure flags (10.4 runtime compatibility)
# We dynamically load X11, so using the system X11 headers is fine.
CONFIG_PPC="--build=`uname -p`-apple-darwin --host=powerpc-apple-darwin \
--x-includes=/usr/X11R6/include --x-libraries=/usr/X11R6/lib"

# PowerPC 32-bit compiler flags
CC_PPC="gcc-4.0 -arch ppc"
CXX_PPC="g++-4.0 -arch ppc"
CFLAGS_PPC="-mmacosx-version-min=10.4"
CPPFLAGS_PPC="-DMAC_OS_X_VERSION_MIN_REQUIRED=1040 \
-nostdinc \
-F$SDK_PATH/MacOSX10.4u.sdk/System/Library/Frameworks \
-I$GCCUSRPATH_PPC/include \
-isystem $SDK_PATH/MacOSX10.4u.sdk/usr/include"

# PowerPC 32-bit linker flags
LFLAGS_PPC="-arch ppc -Wl,-headerpad_max_install_names -mmacosx-version-min=10.4 \
-F$SDK_PATH/MacOSX10.4u.sdk/System/Library/Frameworks \
-L$GCCUSRPATH_PPC \
-Wl,-syslibroot,$SDK_PATH/MacOSX10.4u.sdk"

# PowerPC 64-bit configure flags (10.5 runtime compatibility)
# We dynamically load X11, so using the system X11 headers is fine.
CONFIG_PPC64="--build=`uname -p`-apple-darwin --host=powerpc-apple-darwin \
--x-includes=/usr/X11R6/include --x-libraries=/usr/X11R6/lib"

# PowerPC 64-bit compiler flags
CC_PPC64="gcc-4.0 -arch ppc64"
CXX_PPC64="g++-4.0 -arch ppc64"
CFLAGS_PPC64="-mmacosx-version-min=10.5"
CPPFLAGS_PPC64="-DMAC_OS_X_VERSION_MIN_REQUIRED=1050 \
-nostdinc \
-F$SDK_PATH/MacOSX10.5.sdk/System/Library/Frameworks \
-I$GCCUSRPATH_PPC64/include \
-isystem $SDK_PATH/MacOSX10.5.sdk/usr/include"

# PowerPC 64-bit linker flags
LFLAGS_PPC64="-arch ppc64 -Wl,-headerpad_max_install_names -mmacosx-version-min=10.5 \
-F$SDK_PATH/MacOSX10.5.sdk/System/Library/Frameworks \
-L$GCCUSRPATH_PPC64/ppc64 \
-Wl,-syslibroot,$SDK_PATH/MacOSX10.5.sdk"

# Intel 32-bit configure flags (10.4 runtime compatibility)
# We dynamically load X11, so using the system X11 headers is fine.
CONFIG_X86="--build=`uname -p`-apple-darwin --host=i386-apple-darwin \
--x-includes=/usr/X11R6/include --x-libraries=/usr/X11R6/lib"

# They changed this from "darwin9" to "darwin10" in Xcode 3.2 (Snow Leopard).
GCCUSRPATH_X86=`ls -d $SDK_PATH/MacOSX10.4u.sdk/usr/lib/gcc/i686-apple-darwin*/4.0.1`
if [ ! -d "$GCCUSRPATH_X86" ]; then
    echo "Couldn't find any GCC usr path for 32-bit x86"
    exit 1
fi
GCCUSRPATH_X64=`ls -d $SDK_PATH/MacOSX10.5.sdk/usr/lib/gcc/i686-apple-darwin*/4.0.1`
if [ ! -d "$GCCUSRPATH_X64" ]; then
    echo "Couldn't find any GCC usr path for 64-bit x86"
    exit 1
fi

# Intel 32-bit compiler flags
CC_X86="gcc-4.0 -arch i386"
CXX_X86="g++-4.0 -arch i386"
CFLAGS_X86="-mmacosx-version-min=10.4"
CPPFLAGS_X86="-DMAC_OS_X_VERSION_MIN_REQUIRED=1040 \
-nostdinc \
-F$SDK_PATH/MacOSX10.4u.sdk/System/Library/Frameworks \
-I$GCCUSRPATH_X86/include \
-isystem $SDK_PATH/MacOSX10.4u.sdk/usr/include"

# Intel 32-bit linker flags
LFLAGS_X86="-arch i386 -Wl,-headerpad_max_install_names -mmacosx-version-min=10.4 \
-F$SDK_PATH/MacOSX10.4u.sdk/System/Library/Frameworks \
-L$GCCUSRPATH_X86 \
-Wl,-syslibroot,$SDK_PATH/MacOSX10.4u.sdk"

# Intel 64-bit configure flags (10.5 runtime compatibility)
# We dynamically load X11, so using the system X11 headers is fine.
CONFIG_X64="--build=`uname -p`-apple-darwin --host=i386-apple-darwin \
--x-includes=/usr/X11R6/include --x-libraries=/usr/X11R6/lib"

# Intel 64-bit compiler flags
CC_X64="gcc-4.0 -arch x86_64"
CXX_X64="g++-4.0 -arch x86_64"
CFLAGS_X64="-mmacosx-version-min=10.5"
CPPFLAGS_X64="-DMAC_OS_X_VERSION_MIN_REQUIRED=1050 \
-nostdinc \
-F$SDK_PATH/MacOSX10.5.sdk/System/Library/Frameworks \
-I$GCCUSRPATH_X64/include \
-isystem $SDK_PATH/MacOSX10.5.sdk/usr/include"

# Intel 64-bit linker flags
LFLAGS_X64="-arch x86_64 -Wl,-headerpad_max_install_names -mmacosx-version-min=10.5 \
-F$SDK_PATH/MacOSX10.5.sdk/System/Library/Frameworks \
-L$GCCUSRPATH_X64/x86_64 \
-Wl,-syslibroot,$SDK_PATH/MacOSX10.5.sdk"

#
# Find the configure script
#
srcdir=`dirname $0`/..
auxdir=$srcdir/build-scripts
cd $srcdir

#
# Figure out which phase to build:
# all,
# configure, configure-ppc, configure-ppc64, configure-x86, configure-x64
# make, make-ppc, make-ppc64, make-x86, make-x64, merge
# install
# clean
if test x"$1" = x; then
    phase=all
else
    phase="$1"
fi
case $phase in
    all)
        configure_ppc="yes"
        configure_ppc64="yes"
        configure_x86="yes"
        configure_x64="yes"
        make_ppc="yes"
        make_ppc64="yes"
        make_x86="yes"
        make_x64="yes"
        merge="yes"
        ;;
    configure)
        configure_ppc="yes"
        configure_ppc64="yes"
        configure_x86="yes"
        configure_x64="yes"
        ;;
    configure-ppc)
        configure_ppc="yes"
        ;;
    configure-ppc64)
        configure_ppc64="yes"
        ;;
    configure-x86)
        configure_x86="yes"
        ;;
    configure-x64)
        configure_x64="yes"
        ;;
    make)
        make_ppc="yes"
        make_ppc64="yes"
        make_x86="yes"
        make_x64="yes"
        merge="yes"
        ;;
    make-ppc)
        make_ppc="yes"
        ;;
    make-ppc64)
        make_ppc64="yes"
        ;;
    make-x86)
        make_x86="yes"
        ;;
    make-x64)
        make_x64="yes"
        ;;
    merge)
        merge="yes"
        ;;
    install)
        install_bin="yes"
        install_hdrs="yes"
        install_lib="yes"
        install_data="yes"
        install_man="yes"
        ;;
    install-bin)
        install_bin="yes"
        ;;
    install-hdrs)
        install_hdrs="yes"
        ;;
    install-lib)
        install_lib="yes"
        ;;
    install-data)
        install_data="yes"
        ;;
    install-man)
        install_man="yes"
        ;;
    clean)
        clean_ppc="yes"
        clean_ppc64="yes"
        clean_x86="yes"
        clean_x64="yes"
        ;;
    clean-ppc)
        clean_ppc="yes"
        ;;
    clean-ppc64)
        clean_ppc64="yes"
        ;;
    clean-x86)
        clean_x86="yes"
        ;;
    clean-x64)
        clean_x64="yes"
        ;;
    *)
        echo "Usage: $0 [all|configure[-ppc|-ppc64|-x86|-x64]|make[-ppc|-ppc64|-x86|-x64]|merge|install|clean[-ppc|-ppc64|-x86|-x64]]"
        exit 1
        ;;
esac
case `uname -p` in
    powerpc)
        native_path=ppc
        ;;
    powerpc64)
        native_path=ppc64
        ;;
    *86)
        native_path=x86
        ;;
    x86_64)
        native_path=x64
        ;;
    *)
        echo "Couldn't figure out native architecture path"
        exit 1
        ;;
esac

#
# Create the build directories
#
for dir in build build/ppc build/ppc64 build/x86 build/x64; do
    if test -d $dir; then
        :
    else
        mkdir $dir || exit 1
    fi
done

#
# Build the PowerPC 32-bit binary
#
if test x$configure_ppc = xyes; then
    (cd build/ppc && \
     sh ../../configure $CONFIG_PPC CC="$CC_PPC" CXX="$CXX_PPC" CFLAGS="$CFLAGS $CFLAGS_PPC" CPPFLAGS="$CPPFLAGS_PPC" LDFLAGS="$LFLAGS_PPC") || exit 2
fi
if test x$make_ppc = xyes; then
    (cd build/ppc && ls include && make -j$NJOB) || exit 3
fi

#
# Build the PowerPC 64-bit binary
#
if test x$configure_ppc64 = xyes; then
    (cd build/ppc64 && \
     sh ../../configure $CONFIG_PPC64 CC="$CC_PPC64" CXX="$CXX_PPC64" CFLAGS="$CFLAGS $CFLAGS_PPC64" CPPFLAGS="$CPPFLAGS_PPC64" LDFLAGS="$LFLAGS_PPC64") || exit 2
fi
if test x$make_ppc64 = xyes; then
    (cd build/ppc64 && ls include && make -j$NJOB) || exit 3
fi

#
# Build the Intel 32-bit binary
#
if test x$configure_x86 = xyes; then
    (cd build/x86 && \
     sh ../../configure $CONFIG_X86 CC="$CC_X86" CXX="$CXX_X86" CFLAGS="$CFLAGS $CFLAGS_X86" CPPFLAGS="$CPPFLAGS_X86" LDFLAGS="$LFLAGS_X86") || exit 2
fi
if test x$make_x86 = xyes; then
    (cd build/x86 && make -j$NJOB) || exit 3
fi

#
# Build the Intel 32-bit binary
#
if test x$configure_x64 = xyes; then
    (cd build/x64 && \
     sh ../../configure $CONFIG_X64 CC="$CC_X64" CXX="$CXX_X64" CFLAGS="$CFLAGS $CFLAGS_X64" CPPFLAGS="$CPPFLAGS_X64" LDFLAGS="$LFLAGS_X64") || exit 2
fi
if test x$make_x64 = xyes; then
    (cd build/x64 && make -j$NJOB) || exit 3
fi

#
# Combine into fat binary
#
if test x$merge = xyes; then
    output=.libs
    sh $auxdir/mkinstalldirs build/$output
    cd build
    target=`find . -mindepth 4 -maxdepth 4 -type f -name '*.dylib' | head -1 | sed 's|.*/||'`
    (lipo -create -o $output/$target `find . -mindepth 4 -maxdepth 4 -type f -name "*.dylib"` &&
     ln -sf $target $output/libSDL.dylib &&
     lipo -create -o $output/libSDL.a */build/.libs/libSDL.a &&
     cp $native_path/build/.libs/libSDL.la $output &&
     cp $native_path/build/.libs/libSDL.lai $output &&
     cp $native_path/build/libSDL.la . &&
     lipo -create -o libSDLmain.a */build/libSDLmain.a &&
     echo "Build complete!" &&
     echo "Files can be found in the build directory.") || exit 4
    cd ..
fi

#
# Install
#
do_install()
{
    echo $*
    $* || exit 5
}
if test x$prefix = x; then
    prefix=/usr/local
fi
if test x$exec_prefix = x; then
    exec_prefix=$prefix
fi
if test x$bindir = x; then
    bindir=$exec_prefix/bin
fi
if test x$libdir = x; then
    libdir=$exec_prefix/lib
fi
if test x$includedir = x; then
    includedir=$prefix/include
fi
if test x$datadir = x; then
    datadir=$prefix/share
fi
if test x$mandir = x; then
    mandir=$prefix/man
fi
if test x$install_bin = xyes; then
    do_install sh $auxdir/mkinstalldirs $bindir
    do_install /usr/bin/install -c -m 755 build/$native_path/sdl-config $bindir/sdl-config
fi
if test x$install_hdrs = xyes; then
    do_install sh $auxdir/mkinstalldirs $includedir/SDL
    for src in $srcdir/include/*.h; do \
        file=`echo $src | sed -e 's|^.*/||'`; \
        do_install /usr/bin/install -c -m 644 $src $includedir/SDL/$file; \
    done
    do_install /usr/bin/install -c -m 644 $srcdir/include/SDL_config_macosx.h $includedir/SDL/SDL_config.h
fi
if test x$install_lib = xyes; then
    do_install sh $auxdir/mkinstalldirs $libdir
    do_install sh build/$native_path/libtool --mode=install /usr/bin/install -c  build/libSDL.la $libdir/libSDL.la
    do_install /usr/bin/install -c -m 644 build/libSDLmain.a $libdir/libSDLmain.a
    do_install ranlib $libdir/libSDLmain.a
fi
if test x$install_data = xyes; then
    do_install sh $auxdir/mkinstalldirs $datadir/aclocal
    do_install /usr/bin/install -c -m 644 $srcdir/sdl.m4 $datadir/aclocal/sdl.m4
fi
if test x$install_man = xyes; then
    do_install sh $auxdir/mkinstalldirs $mandir/man3
    for src in $srcdir/docs/man3/*.3; do \
        file=`echo $src | sed -e 's|^.*/||'`; \
        do_install /usr/bin/install -c -m 644 $src $mandir/man3/$file; \
    done
fi

#
# Clean up
#
do_clean()
{
    echo $*
    $* || exit 6
}
if test x$clean_x86 = xyes; then
    do_clean rm -r build/x86
fi
if test x$clean_ppc = xyes; then
    do_clean rm -r build/ppc
fi

