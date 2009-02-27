AC_INIT(ZeroSPU2,0.1,zerofrog@gmail.com)

AM_INIT_AUTOMAKE(ZeroSPU2,0.1)

AC_PROG_CC([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CXX([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CPP([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])

AC_PROG_INSTALL
AC_PROG_RANLIB

dnl necessary for compiling assembly
AM_PROG_AS

AC_SUBST(ZEROSPU2_CURRENT, 0)
AC_SUBST(ZEROSPU2_REVISION, 1)
AC_SUBST(ZEROSPU2_AGE, 0)
AC_SUBST(ZEROSPU2_RELEASE, [$ZEROSPU2_CURRENT].[$ZEROSPU2_REVISION].[$ZEROSPU2_AGE])
AC_SUBST(ZEROSPU2_SONAME, libZeroSPU2.so.[$ZEROSPU2_CURRENT].[$ZEROSPU2_REVISION].[$ZEROSPU2_AGE])

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

dnl Check for dev build
AC_MSG_CHECKING(for development build...)
AC_ARG_ENABLE(devbuild, AC_HELP_STRING([--enable-devbuild], [Special Build for developers that simplifies testing and adds extra checks]),
 devbuild=$enableval,devbuild=no)
if test "x$devbuild" == xyes
then
 AC_DEFINE(ZEROSPU2_DEVBUILD,1,[ZEROSPU2_DEVBUILD])
fi
AC_MSG_RESULT($devbuild)
AM_CONDITIONAL(RELEASE_TO_PUBLIC, test x$devbuild = xno)

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

AC_CHECK_FUNCS([ _aligned_malloc _aligned_free ], AC_DEFINE(HAVE_ALIGNED_MALLOC))

dnl gtk
AC_MSG_CHECKING(gtk2+)
AC_CHECK_PROG(GTK_CONFIG, pkg-config, pkg-config)
LIBS+=$(pkg-config --libs gtk+-2.0)

dnl bindir = pcsx2exe

dnl assuming linux environment
so_ext=".so.$ZEROSPU2_RELEASE"
SHARED_LDFLAGS="-shared"
AC_SUBST(so_ext)
AC_SUBST(SHARED_LDFLAGS)

AC_CHECK_LIB(stdc++,main,[LIBS="$LIBS -lstdc++"])
AC_CHECK_LIB(dl,main,[LIBS="$LIBS -ldl"])
AC_CHECK_LIB(asound,main,[LIBS="$LIBS -lasound"])

AC_OUTPUT([
 Makefile
 ])

echo "Configuration:"
echo "  Debug build?	       $debug"
echo "  Dev build?	       $devbuild"
echo "  x86-64 build?	       $cpu64"