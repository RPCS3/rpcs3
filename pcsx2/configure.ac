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

dnl These optimizations seem to cause issues with GCC 4.3.3, so we'll turn them off. 
dnl I really need to run some tests and see if they are still needed.
  if test "$gccvernum" -ge "433"; then
    NORMAL_FLAGS+=" -fno-guess-branch-probability -fno-dse -fno-tree-dse "
  fi
  
dnl Anything higher then -O0 seem to cause issues with GCC 4.4.0.
  if test "x$optimization" == "xyes"
  then
   dnl Oddly, turning on all the optimizations up to -O2 one by one doesn't cause issues in GCC 4.4.0.
  if test "$gccvernum" -ge "440"; then
   NORMAL_FLAGS+=" -O0 "
    dnl -O1, flag by flag, minus the 3 taken out on gcc 4.3.3
    NORMAL_FLAGS+=" -ftree-builtin-call-dce -ftree-ccp -ftree-ch -ftree-copyrename "
    NORMAL_FLAGS+=" -ftree-dce -ftree-dominator-opts "
    NORMAL_FLAGS+=" -ftree-fre "
    NORMAL_FLAGS+=" -ftree-sra "
    NORMAL_FLAGS+=" -ftree-ter "
    NORMAL_FLAGS+=" -fauto-inc-dec  -fcprop-registers -fdce -fdefer-pop "
    NORMAL_FLAGS+=" -fif-conversion2 -fif-conversion "
    NORMAL_FLAGS+=" -finline-small-functions -fipa-pure-const -fipa-reference -fmerge-constants " 
    NORMAL_FLAGS+=" -fsplit-wide-types -funit-at-a-time -fno-omit-frame-pointer"
    dnl And now for -O2. Comment out one that breaks compilation when turned on individually.
    NORMAL_FLAGS+=" -fthread-jumps -falign-functions -falign-jumps -falign-loops -falign-labels "
    NORMAL_FLAGS+=" -fcaller-saves -fcrossjumping -fcse-follow-jumps -fcse-skip-blocks "
    NORMAL_FLAGS+=" -fdelete-null-pointer-checks -fexpensive-optimizations -fgcse -fgcse-lm "
    NORMAL_FLAGS+=" -findirect-inlining -foptimize-sibling-calls -fpeephole2 -fregmove "
    NORMAL_FLAGS+=" -freorder-blocks -freorder-functions -frerun-cse-after-loop "
    NORMAL_FLAGS+=" -fsched-interblock "
    NORMAL_FLAGS+=" -fsched-spec "
    dnl NORMAL_FLAGS+=" -fschedule-insns "
    NORMAL_FLAGS+=" -fschedule-insns2 "
    NORMAL_FLAGS+=" -fstrict-aliasing -fstrict-overflow "
    NORMAL_FLAGS+=" -ftree-switch-conversion -ftree-pre -ftree-vrp "
  else
    NORMAL_FLAGS+=" -pipe -O2 "
  fi
 else
   NORMAL_FLAGS+=" -O0 "
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

  if test "$gccvernum" -ge "440"; then
    echo
    echo "*Congratulations*! You are running GCC 4.4.0! Thanks to this wonderful fact, you get"
    echo "millions of command line switches, probably seriously slowing down your compilation time."
    echo
    echo "GCC 4.4.0 causes pcsx2 to crash at *any* optimization above 0, yet mysteriously"
    echo "works properly if you add in the flags for all the optimizations yourself, which I have done."
    echo "Enjoy!"
    echo
  fi
  
    
