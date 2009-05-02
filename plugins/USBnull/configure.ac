AC_INIT(USBnull, 0.6,arcum42@gmail.com)

AM_INIT_AUTOMAKE(USBnull,0.6)

AC_PROG_CC([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CXX([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CPP([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])

AC_PROG_INSTALL
AC_PROG_RANLIB

dnl necessary for compiling assembly
AM_PROG_AS

AC_SUBST(USBnull_CURRENT, 0)
AC_SUBST(USBnull_REVISION, 6)
AC_SUBST(USBnull_AGE, 0)
AC_SUBST(USBnull_RELEASE,[$USBnull_CURRENT].[$USBnull_REVISION].[$USBnull_AGE])
AC_SUBST(USBnull_SONAME,libUSBnull.so.[$USBnull_CURRENT].[$USBnull_REVISION].[$USBnull_AGE])

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
 AC_DEFINE(USBnull_DEVBUILD,1,[USBnull_DEVBUILD])
fi
AC_MSG_RESULT($devbuild)
AM_CONDITIONAL(RELEASE_TO_PUBLIC, test x$devbuild = xno)

AC_CHECK_FUNCS([ _aligned_malloc _aligned_free ], AC_DEFINE(HAVE_ALIGNED_MALLOC))

dnl gtk
AC_MSG_CHECKING(gtk2+)
AC_CHECK_PROG(GTK_CONFIG, pkg-config, pkg-config)
LIBS+=$(pkg-config --libs gtk+-2.0)

dnl bindir = pcsx2exe

dnl assuming linux environment
so_ext=".so.$USBnull_RELEASE"
SHARED_LDFLAGS="-shared"
AC_SUBST(so_ext)
AC_SUBST(SHARED_LDFLAGS)

AC_CHECK_LIB(stdc++,main,[LIBS="$LIBS -lstdc++"])

AC_OUTPUT([
 Makefile
 ])

echo "Configuration:"
echo "  Debug build?	       $debug"
echo "  Dev build?	       $devbuild"