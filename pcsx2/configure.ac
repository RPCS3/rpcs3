AC_INIT(pcsx2,0.9.4,zerofrog@gmail.com)

AM_INIT_AUTOMAKE(pcsx2,0.9.4)

AC_PROG_CC([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CXX([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CPP([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])

AC_PROG_RANLIB

dnl necessary for compiling assembly
AM_PROG_AS

CFLAGS=
CPPFLAGS=
CXXFLAGS=
CCASFLAGS=

dnl Check for debug build
AC_MSG_CHECKING(debug build)
AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [debug build]),
 debug=$enableval,debug=no)
if test "x$debug" == xyes
then
 AC_DEFINE(_DEBUG,1,[_DEBUG])
 CFLAGS+=" -g "
 CPPFLAGS+=" -g "
 CXXFLAGS+=" -g "
 CCASFLAGS+=" -D_DEBUG -g "
 MYOBJDIR="Debug"
else
 AC_DEFINE(NDEBUG,1,[NDEBUG])
 CFLAGS+=" -O3 -fomit-frame-pointer "
 CPPFLAGS+=" -O3 -fomit-frame-pointer "
 CXXFLAGS+=" -O3 -fomit-frame-pointer "
 MYOBJDIR="Release"
fi
AC_MSG_RESULT($debug)

AC_CHECK_FUNCS([ _aligned_malloc _aligned_free ], AC_DEFINE(HAVE_ALIGNED_MALLOC))

dnl Check for dev build
AC_MSG_CHECKING(for development build)
AC_ARG_ENABLE(devbuild, AC_HELP_STRING([--enable-devbuild], [Special Build for developers that simplifies testing and adds extra checks]),
 devbuild=$enableval,devbuild=no)
if test "x$devbuild" == xyes
then
 AC_DEFINE(PCSX2_DEVBUILD,1,[PCSX2_DEVBUILD])
 MYOBJDIR=$MYOBJDIR"Dev"
fi
AC_MSG_RESULT($devbuild)

AC_MSG_CHECKING(force sse3 instructions)
AC_ARG_ENABLE(sse3, AC_HELP_STRING([--enable-sse3], [Forces sse3 detection on CPUs]),
 sse3=$enableval,sse3=no)
if test "x$sse3" == xyes
then
 AC_DEFINE(PCSX2_FORCESSE3,1,[PCSX2_DEVBUILD])
fi
AC_MSG_RESULT(sse3)

dnl Check for virtual memory build
AC_MSG_CHECKING(for virtual memory build)
AC_ARG_ENABLE(vmbuild, AC_HELP_STRING([--enable-vmbuild], [Adds low level virtual memory support to run pcsx2 faster]),
 vmbuild=$enableval,vmbuild=no)
if test "x$vmbuild" == xyes
then
 AC_DEFINE(PCSX2_VIRTUAL_MEM,1,[PCSX2_VIRTUAL_MEM])
 MYOBJDIR=$MYOBJDIR"VM"
fi
AC_MSG_RESULT($vmbuild)

dnl Check for recompilation
AC_MSG_CHECKING(for using platform dependent recompilers)
AC_ARG_ENABLE(recbuild, AC_HELP_STRING([--disable-recbuild], [Disable all architecture dependent recompilers]),
 recbuild=$enableval,recbuild=yes)
if test "x$recbuild" == xno
then
 AC_DEFINE(PCSX2_NORECBUILD,1,[PCSX2_NORECBUILD])
fi
AC_MSG_RESULT($recbuild)
AM_CONDITIONAL(RECBUILD, test x$recbuild = xyes)

dnl Check for 64bit CPU
AC_MSG_CHECKING(for a x86-64 CPU)
dnl if test "$build_os" == "target_os"
dnl then
AC_TRY_RUN([
int main()
{
int a = 0;
int*pa = &a;
asm(".intel_syntax\n"
	"mov %%rax, %0\n"
    "mov %%eax, [%%rax]\n"
    ".att_syntax\n"
    : : "r"(pa) : "%rax");
return 0;
}
],cpu64=yes,cpu64=no,)
dnl else
dnl cpu64=no
dnl fi
if test "x$cpu64" == xyes
then
AC_DEFINE(__x86_64__,1,[__x86_64__])
CCASFLAGS="$CCASFLAGS -D__x86_64__"
fi
AC_MSG_RESULT($cpu64)
AM_CONDITIONAL(X86_64, test x$cpu64 = xyes)

dnl gtk
AC_MSG_CHECKING(gtk+)
AC_CHECK_PROG(GTK_CONFIG, pkg-config, pkg-config)
LIBS+=$(pkg-config --libs gtk+-2.0 gthread-2.0)

dnl AC_CHECK_LIB(pthread,main,[LIBS="$LIBS -lpthread"])
AC_CHECK_LIB(stdc++,main,[LIBS="$LIBS -lstdc++"])
AC_CHECK_LIB(z,main,[LIBS="$LIBS -lz"])

AC_CHECK_HEADER([libintl.h], [AC_DEFINE(ENABLE_NLS,1,[__x86_64__])])

AC_OUTPUT([
 Makefile
 DebugTools/Makefile
 Linux/Makefile
 IPU/Makefile
 IPU/mpeg2lib/Makefile
 RDebug/Makefile
 tinyxml/Makefile
 x86/Makefile
 x86/ix86/Makefile
 zlib/Makefile
 ])

dnl bindir = pcsx2exe

echo "Configuration:"
echo "  Target system type:    $target"
echo "  x86-64 build?	       $cpu64"
echo "  Debug build?	       $debug"
echo "  Dev build?	           $devbuild"
echo "  Force sse3?	           $sse3"
echo "  Recompilers enabled?   $recbuild"
echo "  Virtual memory build?  $vmbuild"
