AC_INIT(SPU2X, 0.1,Jake.Stine@gmail.com)

AM_INIT_AUTOMAKE(SPU2X,0.1)

AC_PROG_CC([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CXX([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CPP([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])

AC_PROG_INSTALL
AC_PROG_RANLIB

dnl necessary for compiling assembly
AM_PROG_AS

SPU2X_CURRENT=0
SPU2X_REVISION=1
SPU2X_AGE=0
SPU2X_SONAME=libSPU2X.so.[$SPU2X_CURRENT].[$SPU2X_REVISION].[$SPU2X_AGE]
SPU2X_RELEASE=[$SPU2X_CURRENT].[$SPU2X_REVISION].[$SPU2X_AGE]

AC_SUBST(SPU2X_CURRENT)
AC_SUBST(SPU2X_REVISION)
AC_SUBST(SPU2X_AGE)
AC_SUBST(SPU2X_RELEASE)
AC_SUBST(SPU2X_SONAME)

CFLAGS=
CPPFLAGS=
CXXFLAGS=

dnl Check for debug build
AC_MSG_CHECKING(debug build)
AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [debug build]),
debug=$enableval,debug=no)
if test "x$debug" == xyes
then
 AC_DEFINE(_DEBUG,1,[_DEBUG])
 CFLAGS+="-g -fPIC -Wall  -Wno-unused-value "
 CPPFLAGS+="-g -fPIC -Wall  -Wno-unused-value "
 CXXFLAGS+="-g -fPIC -Wall  -Wno-unused-value "
else
 AC_DEFINE(NDEBUG,1,[NDEBUG])
 CFLAGS+="-O3 -fomit-frame-pointer -fPIC -Wall  -Wno-unused-value "
 CPPFLAGS+="-O3 -fomit-frame-pointer -fPIC -Wall  -Wno-unused-value "
 CXXFLAGS+="-O3 -fomit-frame-pointer -fPIC -Wall  -Wno-unused-value "
fi
AM_CONDITIONAL(DEBUGBUILD, test x$debug = xyes)
AC_MSG_RESULT($debug)

AC_DEFINE(__LINUX__,1,[__LINUX__])

dnl Check for dev build
AC_MSG_CHECKING(for development build...)
AC_ARG_ENABLE(devbuild, AC_HELP_STRING([--enable-devbuild], [Special Build for developers that simplifies testing and adds extra checks]),
 devbuild=$enableval,devbuild=no)
if test "x$devbuild" == xyes
then
 AC_DEFINE(SPU2X_DEVBUILD,1,[SPU2X_DEVBUILD])
fi
AC_MSG_RESULT($devbuild)
AM_CONDITIONAL(RELEASE_TO_PUBLIC, test x$devbuild = xno)

AC_CHECK_FUNCS([ _aligned_malloc _aligned_free ], AC_DEFINE(HAVE_ALIGNED_MALLOC))

dnl gtk
AC_MSG_CHECKING(gtk2+)
AC_CHECK_PROG(GTK_CONFIG, pkg-config, pkg-config)
LIBS+=$(pkg-config --libs gtk+-2.0)

dnl bindir = pcsx2exe

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
fi
AC_MSG_RESULT($cpu64)
AM_CONDITIONAL(X86_64, test x$cpu64 = xyes)

dnl assuming linux environment
so_ext=".so.$SPU2X_RELEASE"
SHARED_LDFLAGS="-shared"
AC_SUBST(so_ext)
AC_SUBST(SHARED_LDFLAGS)
AC_CHECK_FUNCS(iconv, [ have_iconv="1" ], [ have_iconv="0" ])

AC_CHECK_LIB(stdc++,main,[LIBS="$LIBS -lstdc++"])
AC_CHECK_LIB(dl,main,[LIBS="$LIBS -ldl"])
AC_CHECK_LIB(asound,main,[LIBS="$LIBS -lasound"])

AC_OUTPUT([
 3rdparty/SoundTouch/Makefile
 Makefile
 ])

echo "Configuration:"
echo "  Debug build?	       $debug"
echo "  Dev build?	       $devbuild"