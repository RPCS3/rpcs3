AC_INIT(ZZOgl-pg,0.1.0,arcum42@gmail.com)

AM_INIT_AUTOMAKE(ZZOgl-pg,0.1.0)

AC_PROG_CC([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CXX([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AC_PROG_CPP([gcc g++ cl KCC CC cxx cc++ xlC aCC c++])
AM_PROG_CC_C_O

AC_PROG_INSTALL
AC_PROG_RANLIB

dnl necessary for compiling assembly
AM_PROG_AS

ZEROGS_CURRENT=0
ZEROGS_REVISION=1
ZEROGS_AGE=0
ZEROGS_SONAME=libzzoglpg.so.[$ZEROGS_CURRENT].[$ZEROGS_REVISION].[$ZEROGS_AGE]
ZEROGS_RELEASE=[$ZEROGS_CURRENT].[$ZEROGS_REVISION].[$ZEROGS_AGE]

AC_SUBST(ZEROGS_CURRENT)
AC_SUBST(ZEROGS_REVISION)
AC_SUBST(ZEROGS_AGE)
AC_SUBST(ZEROGS_RELEASE)
AC_SUBST(ZEROGS_SONAME)

CFLAGS=" -I/opt/cg/include -L/opt/cg/lib -Wall -Wno-format -Wno-unused "
CPPFLAGS=" -I/opt/cg/include -L/opt/cg/lib -Wall -Wno-format -Wno-unused "
CXXFLAGS=" -I/opt/cg/include -L/opt/cg/lib -Wall -Wno-format -Wno-unused "

dnl Check for debug build
AC_MSG_CHECKING(debug build)
AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [debug build]),
 debug=$enableval,debug=no)
if test "x$debug" == xyes
then
 AC_DEFINE(_DEBUG,1,[_DEBUG])
 CFLAGS+="-g "
 CPPFLAGS+="-g "
 CXXFLAGS+="-g -fpermissive "
else
 AC_DEFINE(NDEBUG,1,[NDEBUG])
 CFLAGS+="-O2 -msse2 "
 CPPFLAGS+="-O2 -msse2 "
 CXXFLAGS+="-O2 -msse2 -fno-regmove "
fi
AC_MSG_RESULT($debug)

AC_CHECK_FUNCS([ _aligned_malloc _aligned_free ], AC_DEFINE(HAVE_ALIGNED_MALLOC))

dnl Check for dev build
AC_MSG_CHECKING(for development build...)
AC_ARG_ENABLE(devbuild, AC_HELP_STRING([--enable-devbuild], [Special Build for developers that simplifies testing and adds extra checks]),
 devbuild=$enableval,devbuild=no)
if test "x$devbuild" == xno
then
 AC_DEFINE(RELEASE_TO_PUBLIC,1,[RELEASE_TO_PUBLIC])
fi
AC_MSG_RESULT($devbuild)
AM_CONDITIONAL(RELEASE_TO_PUBLIC, test x$devbuild = xno)

dnl Check for recompilation
AC_MSG_CHECKING(check for sse2...)
AC_ARG_ENABLE(sse2, AC_HELP_STRING([--enable-sse2], [Enable sse2 instructions]),
 sse2=$enableval,sse2=no)
if test "x$sse2" == xyes
then
 AC_DEFINE(ZEROGS_SSE2,1,[ZEROGS_SSE2])
fi
AC_MSG_RESULT($sse2)
AM_CONDITIONAL(SSE2, test x$sse2 = xyes)

dnl gtk
AC_MSG_CHECKING(gtk2+)
AC_CHECK_PROG(GTK_CONFIG, pkg-config, pkg-config)
LIBS+=$(pkg-config --libs gtk+-2.0)

dnl bindir = pcsx2exe

dnl assuming linux environment
so_ext=".so.$ZEROGS_RELEASE"
SHARED_LDFLAGS="-shared"
AC_SUBST(so_ext)
AC_SUBST(SHARED_LDFLAGS)

AC_MSG_CHECKING(OpenGL)
AC_CHECK_HEADERS([GL/gl.h GL/glu.h GL/glext.h],,,
      [[#if HAVE_GL_GL_H
         #include <GL/gl.h>
         #endif
  	   #if HAVE_GL_GLU_H
         #include <GL/glu.h>
         #endif
       ]])
AC_CHECK_LIB(GL,main,[LIBS="$LIBS -lGL"])
AC_CHECK_LIB(GLU,main,[LIBS="$LIBS -lGLU"])
AC_CHECK_LIB(GLEW,main,[LIBS="$LIBS -lGLEW"])
AC_MSG_CHECKING(Cg)
AC_CHECK_HEADERS([Cg/cg.h Cg/cgGL.h])
AC_CHECK_LIB(jpeg,main,[LIBS="$LIBS -ljpeg"])
AC_CHECK_HEADERS([jpeglib.h],,[AC_MSG_ERROR(Install libjpeg-dev)])
AC_CHECK_LIB(pthread,main,[LIBS="$LIBS -lpthread"])
AC_CHECK_LIB(stdc++,main,[LIBS="$LIBS -lstdc++"])
AC_CHECK_LIB(z,main,[LIBS="$LIBS -lz"])
AC_CHECK_LIB(dl,main,[LIBS="$LIBS -ldl"])
AC_CHECK_LIB(Xxf86vm,main,[LIBS="$LIBS -lXxf86vm"],[echo "Error! You need to have libXxf86vm-dev." ; exit -1])

dnl AC_CHECK_LIB(Cg,cgCreateContext,[LIBS="$LIBS -lCg"])
dnl AC_CHECK_LIB(CgGL,cgGLEnableProfile,[LIBS="$LIBS -lCgGL"])
LIBS+=" -lCg -lCgGL"

AC_OUTPUT([
 Makefile
 Linux/Makefile
 ZeroGSShaders/Makefile
 ])

echo "Configuration:"
echo "  Target system type:    $target"
echo "  Debug build?	       $debug"
echo "  Dev build?	           $devbuild"
echo "  SSE2 enabled?		   $sse2"
