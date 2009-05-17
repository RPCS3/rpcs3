AC_INIT(pcsx2,0.9.6,arcum42@gmail.com)
AM_INIT_AUTOMAKE(pcsx2,0.9.6)

AC_PROG_CC([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CXX([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CPP([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])

AC_PROG_RANLIB

dnl necessary for compiling assembly
AM_PROG_AS
AC_CONFIG_MACRO_DIR([m4])

if test "x$GCC" = "xyes"; then
  dnl get gcc version
  AC_MSG_CHECKING([gcc version])
  gccver=$($CC -dumpversion)
  gccvermajor=$(echo $gccver | cut -d . -f1)
  gccverminor=$(echo $gccver | cut -d . -f2)
  gccverrevision=$(echo $gccver | cut -d . -f3)
  gccvernum=$(expr $gccvermajor "*" 100 + $gccverminor "*" 10 + $gccverrevision)
  AC_MSG_RESULT($gccver)
fi

AC_ARG_ENABLE(warnings, AC_HELP_STRING([--enable-warnings], [Add warning flags when compiling]),
warnings=$enableval,warnings=no)

AC_ARG_ENABLE(customcflags, AC_HELP_STRING([--enable-customcflags], [Use custom CFLAGS]),
customcflags=$enableval,customcflags=no)

AC_ARG_ENABLE(optimization, AC_HELP_STRING([--disable-optimization], [No optimizations when compiling]),
optimization=$enableval,optimization=yes)

AC_ARG_ENABLE(unsupportedspeedfreak, AC_HELP_STRING([--enable-unsupportedspeedfreak], [Set the compiler to do unreasonable optimizations that will probably make all your games crash.]),
unsupportedspeedfreak=$enableval,unsupportedspeedfreak=no)

dnl Check for debug build
AC_MSG_CHECKING(debug build)
AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [debug build]),
 debug=$enableval,debug=no)
 
if test "x$debug" == "xyes"
then
  AC_DEFINE(_DEBUG,1,[_DEBUG])
  AC_DEFINE(PCSX2_DEVBUILD,1,[PCSX2_DEVBUILD])
  MYOBJDIR="Debug"
  optimization="no"
  warnings="yes"
else
  AC_DEFINE(NDEBUG,1,[NDEBUG])
  MYOBJDIR="Release"
fi
AC_MSG_RESULT($debug)

dnl Check for dev build
AC_MSG_CHECKING(for development build)
AC_ARG_ENABLE(devbuild, AC_HELP_STRING([--enable-devbuild], [Special Build for developers that simplifies testing and adds extra checks]),
 devbuild=$enableval,devbuild=no)
 
if test "x$debug" == "xyes"
then
  dnl Force devbuild on, because Debug mode never seems to stay working properly without Devbuild on.
  devbuild=yes
fi
if test "x$devbuild" == "xyes"
then
  AC_DEFINE(PCSX2_DEVBUILD,1,[PCSX2_DEVBUILD])
  MYOBJDIR="Dev"
  nowarnings="no"
fi
AC_MSG_RESULT($devbuild)

AC_ARG_ENABLE(nls, AC_HELP_STRING([--disable-nls], [Disable nls support]),
nls=$enableval,nls=yes)

if test "x$nls" == xyes
then
  AC_CHECK_HEADER([libintl.h], [AC_DEFINE(ENABLE_NLS,1,[__x86_64__])])
fi

AC_ARG_ENABLE(local-inis, AC_HELP_STRING([--enable-local-inis], [Enable local plugin inis]),
localinis=$enableval, localinis=no)

if test "x$localinis" == xyes
then
  AC_DEFINE(LOCAL_PLUGIN_INIS)
fi

svnrev="`svn info | grep Revision:`"

if test -n "${svnrev}" ; then
  AC_REVISION("${svnrev}") 
  AC_DEFINE_UNQUOTED(SVN_REV,"${svnrev}",
    [Define to be the subversion revision number])
else
  AC_REVISION([$Revision: 0 $])
  AC_DEFINE(SVN_REV,"$Revision: 0 $",
    [Define to be the subversion revision number])
fi

if test "x$customcflags" == "xno"
then
  CFLAGS=
  CPPFLAGS=
  CXXFLAGS=
  CCASFLAGS=
fi

WARNING_FLAGS=
NORMAL_FLAGS=

if test "x$warnings" == "xyes"
then
  WARNING_FLAGS+=" -Wall -Wextra -Wunused-variable "
fi

WARNING_FLAGS+=" -Wno-format -Wno-unused-parameter -Wno-unused-value "
NORMAL_FLAGS+=" -msse -msse2 ${WARNING_FLAGS} "

if test "x$unsupportedspeedfreak" == "xyes"
then
    dnl Don't even try to get support for this.
    NORMAL_FLAGS+=" -pipe -O3 -march=native -funroll-loops -ffast-math "
else
  if test "x$optimization" == "xyes"
  then
    NORMAL_FLAGS+=" -pipe -O2 "
  else
    NORMAL_FLAGS+=" -O0 "
  fi
fi

if test "x$unsupportedspeedfreak" == "xno"
then
dnl These optimizations seem to cause issues with GCC 4.3.3, so we'll turn them off. 
dnl I really need to run some tests and see if they are still needed.
    if test "$gccvernum" -ge "433"; then
      NORMAL_FLAGS+=" -fno-guess-branch-probability -fno-dse -fno-tree-dse "
    fi
fi

 CFLAGS+=" ${NORMAL_FLAGS} -Xlinker -zmuldefs -m32 "
 CXXFLAGS+=" ${NORMAL_FLAGS} -fpermissive -Xlinker -zmuldefs -m32 "
 CCASFLAGS+=" ${NORMAL_FLAGS}  -m32 "

if test "x$debug" == "xyes"
then
 CFLAGS+=" -g "
 CXXFLAGS+=" -g "
 CCASFLAGS+=" -D_DEBUG "
fi

AC_CHECK_FUNCS([ _aligned_malloc _aligned_free ], AC_DEFINE(HAVE_ALIGNED_MALLOC))

dnl gtk
AC_MSG_CHECKING(gtk+)
AC_CHECK_PROG(GTK_CONFIG, pkg-config, pkg-config)
LIBS+=$(pkg-config --libs gtk+-2.0 gthread-2.0)

dnl AC_CHECK_LIB(pthread,main,[LIBS="$LIBS -lpthread"])
AC_CHECK_LIB(stdc++,main,[LIBS="$LIBS -lstdc++"])
AC_CHECK_LIB(z,main,[LIBS="$LIBS -lz"])

AC_OUTPUT([
 Makefile
 CDVD/Makefile
 DebugTools/Makefile
 Linux/Makefile
 IPU/Makefile
 IPU/mpeg2lib/Makefile
 RDebug/Makefile
 tinyxml/Makefile
 x86/Makefile
 x86/ix86/Makefile
 ../3rdparty/zlib/Makefile
 ])

dnl bindir = pcsx2exe

echo "Configuration:"
echo "  Target system type: $target"
echo "  Debug build? $debug"
echo "  Dev build? $devbuild"
echo "  NLS support? $nls"
echo "  Local plugin inis? $localinis"
echo "  Custom cflags? $customcflags"
echo "  Compiler warnings? $warnings"
echo "  Compiler optimizations? $optimization"

