AC_INIT(ZeroPAD,0.3,zerofrog@gmail.com)

AM_INIT_AUTOMAKE(ZeroPAD,0.3)

AC_PROG_CC([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CXX([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CPP([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])

AC_PROG_INSTALL
AC_PROG_RANLIB

dnl necessary for compiling assembly
AM_PROG_AS

AC_SUBST(ZEROPAD_CURRENT, 0)
AC_SUBST(ZEROPAD_REVISION, 3)
AC_SUBST(ZEROPAD_AGE, 0)
AC_SUBST(ZEROPAD_RELEASE, [$ZEROPAD_CURRENT].[$ZEROPAD_REVISION].[$ZEROPAD_AGE])
AC_SUBST(ZEROPAD_SONAME, libZeroPAD.so.[$ZEROPAD_CURRENT].[$ZEROPAD_REVISION].[$ZEROPAD_AGE])

CFLAGS=
CXXFLAGS=
CCASFLAGS=

dnl Check for debug build
AC_MSG_CHECKING(debug build)
AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [debug build]),
 debug=$enableval,debug=no)
if test "x$debug" == xyes
then
 AC_DEFINE(PCSX2_DEBUG,1,[PCSX2_DEBUG])
 CFLAGS+="-g -m32 -fpic "
 CXXFLAGS+="-g -m32 -fpic "
 CCASFLAGS+=" -m32 -fpic "
else
 AC_DEFINE(NDEBUG,1,[NDEBUG])
 CFLAGS+="-O2 -fomit-frame-pointer -m32 -fpic "
 CXXFLAGS+="-O2 -fomit-frame-pointer -m32 -fpic "
 CCASFLAGS+=" -m32 -fpic "
fi
AM_CONDITIONAL(DEBUGBUILD, test x$debug = xyes)
AC_MSG_RESULT($debug)


AC_CHECK_FUNCS([ _aligned_malloc _aligned_free ], AC_DEFINE(HAVE_ALIGNED_MALLOC))

AC_CHECK_LIB(SDL,SDL_Init,[])

dnl gtk
AC_MSG_CHECKING(gtk2+)
AC_CHECK_PROG(GTK_CONFIG, pkg-config, pkg-config)
LIBS+=$(pkg-config --libs gtk+-2.0 sdl)
CFLAGS+="$(pkg-config --cflags gtk+-2.0 sdl) "
CXXFLAGS+="$(pkg-config --cflags gtk+-2.0 sdl) "

dnl AC_CHECK_HEADER([SDL/SDL.h], [AC_DEFINE(JOYSTICK_SUPPORT,1)])

dnl assuming linux environment
so_ext=".so.$ZEROPAD_RELEASE"
SHARED_LDFLAGS="-shared"
AC_SUBST(so_ext)
AC_SUBST(SHARED_LDFLAGS)

AC_CHECK_LIB(stdc++,main,[LIBS="$LIBS -lstdc++"])
AC_CHECK_LIB(dl,main,[LIBS="$LIBS -ldl"])


AC_OUTPUT([
 Makefile
 ])

echo "Configuration:"
echo "  Debug build?	       $debug"
echo "  x86-64 build?	       $cpu64"