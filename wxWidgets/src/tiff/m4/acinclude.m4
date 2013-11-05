dnl ---------------------------------------------------------------------------
dnl Message output
dnl ---------------------------------------------------------------------------
AC_DEFUN([LOC_MSG],[echo "$1"])

dnl ---------------------------------------------------------------------------
dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/vl_prog_cc_warnings.html
dnl ---------------------------------------------------------------------------

dnl @synopsis VL_PROG_CC_WARNINGS([ANSI])
dnl
dnl Enables a reasonable set of warnings for the C compiler.
dnl Optionally, if the first argument is nonempty, turns on flags which
dnl enforce and/or enable proper ANSI C if such are known with the
dnl compiler used.
dnl
dnl Currently this macro knows about GCC, Solaris C compiler, Digital
dnl Unix C compiler, C for AIX Compiler, HP-UX C compiler, IRIX C
dnl compiler, NEC SX-5 (Super-UX 10) C compiler, and Cray J90 (Unicos
dnl 10.0.0.8) C compiler.
dnl
dnl @category C
dnl @author Ville Laurikari <vl@iki.fi>
dnl @version 2002-04-04
dnl @license AllPermissive

AC_DEFUN([VL_PROG_CC_WARNINGS], [
  ansi=$1
  if test -z "$ansi"; then
    msg="for C compiler warning flags"
  else
    msg="for C compiler warning and ANSI conformance flags"
  fi
  AC_CACHE_CHECK($msg, vl_cv_prog_cc_warnings, [
    if test -n "$CC"; then
      cat > conftest.c <<EOF
int main(int argc, char **argv) { return 0; }
EOF

      dnl GCC. -W option has been renamed in -wextra in latest gcc versions.
      if test "$GCC" = "yes"; then
        if test -z "$ansi"; then
          vl_cv_prog_cc_warnings="-Wall -W"
        else
          vl_cv_prog_cc_warnings="-Wall -W -ansi -pedantic"
        fi

      dnl Most compilers print some kind of a version string with some command
      dnl line options (often "-V").  The version string should be checked
      dnl before doing a test compilation run with compiler-specific flags.
      dnl This is because some compilers (like the Cray compiler) only
      dnl produce a warning message for unknown flags instead of returning
      dnl an error, resulting in a false positive.  Also, compilers may do
      dnl erratic things when invoked with flags meant for a different
      dnl compiler.

      dnl Solaris C compiler
      elif $CC -V 2>&1 | grep -i "WorkShop" > /dev/null 2>&1 &&
           $CC -c -v -Xc conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          vl_cv_prog_cc_warnings="-v"
        else
          vl_cv_prog_cc_warnings="-v -Xc"
        fi

      dnl Digital Unix C compiler
      elif $CC -V 2>&1 | grep -i "Digital UNIX Compiler" > /dev/null 2>&1 &&
           $CC -c -verbose -w0 -warnprotos -std1 conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          vl_cv_prog_cc_warnings="-verbose -w0 -warnprotos"
        else
          vl_cv_prog_cc_warnings="-verbose -w0 -warnprotos -std1"
        fi

      dnl C for AIX Compiler
      elif $CC 2>&1 | grep -i "C for AIX Compiler" > /dev/null 2>&1 &&
           $CC -c -qlanglvl=ansi -qinfo=all conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          vl_cv_prog_cc_warnings="-qsrcmsg -qinfo=all:noppt:noppc:noobs:nocnd"
        else
          vl_cv_prog_cc_warnings="-qsrcmsg -qinfo=all:noppt:noppc:noobs:nocnd -qlanglvl=ansi"
        fi

      dnl IRIX C compiler
      elif $CC -version 2>&1 | grep -i "MIPSpro Compilers" > /dev/null 2>&1 &&
           $CC -c -fullwarn -ansi -ansiE conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          vl_cv_prog_cc_warnings="-fullwarn"
        else
          vl_cv_prog_cc_warnings="-fullwarn -ansi -ansiE"
        fi

      dnl HP-UX C compiler
      elif what $CC 2>&1 | grep -i "HP C Compiler" > /dev/null 2>&1 &&
           $CC -c -Aa +w1 conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          vl_cv_prog_cc_warnings="+w1"
        else
          vl_cv_prog_cc_warnings="+w1 -Aa"
        fi

      dnl The NEC SX-5 (Super-UX 10) C compiler
      elif $CC -V 2>&1 | grep "/SX" > /dev/null 2>&1 &&
           $CC -c -pvctl[,]fullmsg -Xc conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          vl_cv_prog_cc_warnings="-pvctl[,]fullmsg"
        else
          vl_cv_prog_cc_warnings="-pvctl[,]fullmsg -Xc"
        fi

      dnl The Cray C compiler (Unicos)
      elif $CC -V 2>&1 | grep -i "Cray" > /dev/null 2>&1 &&
           $CC -c -h msglevel 2 conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          vl_cv_prog_cc_warnings="-h msglevel 2"
        else
          vl_cv_prog_cc_warnings="-h msglevel 2 -h conform"
        fi

      fi
      rm -f conftest.*
    fi
    if test -n "$vl_cv_prog_cc_warnings"; then
      CFLAGS="$CFLAGS $vl_cv_prog_cc_warnings"
    else
      vl_cv_prog_cc_warnings="unknown"
    fi
  ])
])dnl

dnl ---------------------------------------------------------------------------
dnl Available from the GNU Autoconf Macro Archive at:
dnl http://autoconf-archive.cryp.to/ax_lang_compiler_ms.html
dnl ---------------------------------------------------------------------------

dnl @synopsis AX_LANG_COMPILER_MS
dnl
dnl Check whether the compiler for the current language is Microsoft.
dnl
dnl This macro is modeled after _AC_LANG_COMPILER_GNU in the GNU
dnl Autoconf implementation.
dnl
dnl @category InstalledPackages
dnl @author Braden McDaniel <braden@endoframe.com>
dnl @version 2004-11-15
dnl @license AllPermissive

AC_DEFUN([AX_LANG_COMPILER_MS],
[AC_CACHE_CHECK([whether we are using the Microsoft _AC_LANG compiler],
                [ax_cv_[]_AC_LANG_ABBREV[]_compiler_ms],
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[#ifndef _MSC_VER
       choke me
#endif
]])],
                   [ax_compiler_ms=yes],
                   [ax_compiler_ms=no])
ax_cv_[]_AC_LANG_ABBREV[]_compiler_ms=$ax_compiler_ms
])])

dnl ---------------------------------------------------------------------------
dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/ax_check_gl.html
dnl ---------------------------------------------------------------------------

dnl SYNOPSIS
dnl
dnl   AX_CHECK_GL
dnl
dnl DESCRIPTION
dnl
dnl   Check for an OpenGL implementation. If GL is found, the required
dnl   compiler and linker flags are included in the output variables
dnl   "GL_CFLAGS" and "GL_LIBS", respectively. If no usable GL implementation
dnl   is found, "no_gl" is set to "yes".
dnl
dnl   If the header "GL/gl.h" is found, "HAVE_GL_GL_H" is defined. If the
dnl   header "OpenGL/gl.h" is found, HAVE_OPENGL_GL_H is defined. These
dnl   preprocessor definitions may not be mutually exclusive.
dnl
dnl LICENSE
dnl
dnl   Copyright (c) 2009 Braden McDaniel <braden@endoframe.com>
dnl
dnl   This program is free software; you can redistribute it and/or modify it
dnl   under the terms of the GNU General Public License as published by the
dnl   Free Software Foundation; either version 2 of the License, or (at your
dnl   option) any later version.
dnl
dnl   This program is distributed in the hope that it will be useful, but
dnl   WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
dnl   Public License for more details.
dnl
dnl   You should have received a copy of the GNU General Public License along
dnl   with this program. If not, see <http://www.gnu.org/licenses/>.
dnl
dnl   As a special exception, the respective Autoconf Macro's copyright owner
dnl   gives unlimited permission to copy, distribute and modify the configure
dnl   scripts that are the output of Autoconf when processing the Macro. You
dnl   need not follow the terms of the GNU General Public License when using
dnl   or distributing such scripts, even though portions of the text of the
dnl   Macro appear in them. The GNU General Public License (GPL) does govern
dnl   all other use of the material that constitutes the Autoconf Macro.
dnl
dnl   This special exception to the GPL applies to versions of the Autoconf
dnl   Macro released by the Autoconf Archive. When you make and distribute a
dnl   modified version of the Autoconf Macro, you may extend this special
dnl   exception to the GPL to apply to your modified version as well.

AC_DEFUN([AX_CHECK_GL],
[AC_REQUIRE([AC_CANONICAL_HOST])
AC_REQUIRE([AC_PATH_X])dnl
AC_REQUIRE([AX_PTHREAD])dnl

AC_LANG_PUSH([C])
AX_LANG_COMPILER_MS
AS_IF([test X$ax_compiler_ms = Xno],
      [GL_CFLAGS="${PTHREAD_CFLAGS}"; GL_LIBS="${PTHREAD_LIBS} -lm"])

dnl
dnl Use x_includes and x_libraries if they have been set (presumably by
dnl AC_PATH_X).
dnl
AS_IF([test "X$no_x" != "Xyes"],
      [AS_IF([test -n "$x_includes"],
             [GL_CFLAGS="-I${x_includes} ${GL_CFLAGS}"])]
       AS_IF([test -n "$x_libraries"],
             [GL_LIBS="-L${x_libraries} -lX11 ${GL_LIBS}"]))

ax_save_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS="${GL_CFLAGS} ${CPPFLAGS}"
AC_CHECK_HEADERS([GL/gl.h OpenGL/gl.h])
CPPFLAGS="${ax_save_CPPFLAGS}"

AC_CHECK_HEADERS([windows.h])

m4_define([AX_CHECK_GL_PROGRAM],
          [AC_LANG_PROGRAM([[
# if defined(HAVE_WINDOWS_H) && defined(_WIN32)
#   include <windows.h>
# endif
# ifdef HAVE_GL_GL_H
#   include <GL/gl.h>
# elif defined(HAVE_OPENGL_GL_H)
#   include <OpenGL/gl.h>
# else
#   error no gl.h
# endif]],
                           [[glBegin(0)]])])

AC_CACHE_CHECK([for OpenGL library], [ax_cv_check_gl_libgl],
[ax_cv_check_gl_libgl="no"
case $host_cpu in
  x86_64) ax_check_gl_libdir=lib64 ;;
  *)      ax_check_gl_libdir=lib ;;
esac
ax_save_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS="${GL_CFLAGS} ${CPPFLAGS}"
ax_save_LIBS="${LIBS}"
LIBS=""
ax_check_libs="-lopengl32 -lGL"
for ax_lib in ${ax_check_libs}; do
  AS_IF([test X$ax_compiler_ms = Xyes],
        [ax_try_lib=`echo $ax_lib | sed -e 's/^-l//' -e 's/$/.lib/'`],
        [ax_try_lib="${ax_lib}"])
  LIBS="${ax_try_lib} ${GL_LIBS} ${ax_save_LIBS}"
AC_LINK_IFELSE([AX_CHECK_GL_PROGRAM],
               [ax_cv_check_gl_libgl="${ax_try_lib}"; break],
               [ax_check_gl_nvidia_flags="-L/usr/${ax_check_gl_libdir}/nvidia" LIBS="${ax_try_lib} ${ax_check_gl_nvidia_flags} ${GL_LIBS} ${ax_save_LIBS}"
AC_LINK_IFELSE([AX_CHECK_GL_PROGRAM],
               [ax_cv_check_gl_libgl="${ax_try_lib} ${ax_check_gl_nvidia_flags}"; break],
               [ax_check_gl_dylib_flag='-dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib' LIBS="${ax_try_lib} ${ax_check_gl_dylib_flag} ${GL_LIBS} ${ax_save_LIBS}"
AC_LINK_IFELSE([AX_CHECK_GL_PROGRAM],
               [ax_cv_check_gl_libgl="${ax_try_lib} ${ax_check_gl_dylib_flag}"; break])])])
done

AS_IF([test "X$ax_cv_check_gl_libgl" = Xno -a "X$no_x" = Xyes],
[LIBS='-framework OpenGL'
AC_LINK_IFELSE([AX_CHECK_GL_PROGRAM],
               [ax_cv_check_gl_libgl="$LIBS"])])

LIBS=${ax_save_LIBS}
CPPFLAGS=${ax_save_CPPFLAGS}])

AS_IF([test "X$ax_cv_check_gl_libgl" = Xno],
      [no_gl=yes; GL_CFLAGS=""; GL_LIBS=""],
      [GL_LIBS="${ax_cv_check_gl_libgl} ${GL_LIBS}"])
AC_LANG_POP([C])

AC_SUBST([GL_CFLAGS])
AC_SUBST([GL_LIBS])
])dnl

dnl ---------------------------------------------------------------------------
dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/ax_check_glu.html
dnl ---------------------------------------------------------------------------

dnl SYNOPSIS
dnl
dnl   AX_CHECK_GLU
dnl
dnl DESCRIPTION
dnl
dnl   Check for GLU. If GLU is found, the required preprocessor and linker
dnl   flags are included in the output variables "GLU_CFLAGS" and "GLU_LIBS",
dnl   respectively. If no GLU implementation is found, "no_glu" is set to
dnl   "yes".
dnl
dnl   If the header "GL/glu.h" is found, "HAVE_GL_GLU_H" is defined. If the
dnl   header "OpenGL/glu.h" is found, HAVE_OPENGL_GLU_H is defined. These
dnl   preprocessor definitions may not be mutually exclusive.
dnl
dnl   Some implementations (in particular, some versions of Mac OS X) are
dnl   known to treat the GLU tesselator callback function type as "GLvoid
dnl   (*)(...)" rather than the standard "GLvoid (*)()". If the former
dnl   condition is detected, this macro defines "HAVE_VARARGS_GLU_TESSCB".
dnl
dnl LICENSE
dnl
dnl   Copyright (c) 2009 Braden McDaniel <braden@endoframe.com>
dnl
dnl   This program is free software; you can redistribute it and/or modify it
dnl   under the terms of the GNU General Public License as published by the
dnl   Free Software Foundation; either version 2 of the License, or (at your
dnl   option) any later version.
dnl
dnl   This program is distributed in the hope that it will be useful, but
dnl   WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
dnl   Public License for more details.
dnl
dnl   You should have received a copy of the GNU General Public License along
dnl   with this program. If not, see <http://www.gnu.org/licenses/>.
dnl
dnl   As a special exception, the respective Autoconf Macro's copyright owner
dnl   gives unlimited permission to copy, distribute and modify the configure
dnl   scripts that are the output of Autoconf when processing the Macro. You
dnl   need not follow the terms of the GNU General Public License when using
dnl   or distributing such scripts, even though portions of the text of the
dnl   Macro appear in them. The GNU General Public License (GPL) does govern
dnl   all other use of the material that constitutes the Autoconf Macro.
dnl
dnl   This special exception to the GPL applies to versions of the Autoconf
dnl   Macro released by the Autoconf Archive. When you make and distribute a
dnl   modified version of the Autoconf Macro, you may extend this special
dnl   exception to the GPL to apply to your modified version as well.

AC_DEFUN([AX_CHECK_GLU],
[AC_REQUIRE([AX_CHECK_GL])dnl
AC_REQUIRE([AC_PROG_CXX])dnl
GLU_CFLAGS="${GL_CFLAGS}"

ax_save_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS="${GL_CFLAGS} ${CPPFLAGS}"
AC_CHECK_HEADERS([GL/glu.h OpenGL/glu.h])
CPPFLAGS="${ax_save_CPPFLAGS}"

m4_define([AX_CHECK_GLU_PROGRAM],
          [AC_LANG_PROGRAM([[
# if defined(HAVE_WINDOWS_H) && defined(_WIN32)
#   include <windows.h>
# endif
# ifdef HAVE_GL_GLU_H
#   include <GL/glu.h>
# elif defined(HAVE_OPENGL_GLU_H)
#   include <OpenGL/glu.h>
# else
#   error no glu.h
# endif]],
                           [[gluBeginCurve(0)]])])

AC_CACHE_CHECK([for OpenGL Utility library], [ax_cv_check_glu_libglu],
[ax_cv_check_glu_libglu="no"
ax_save_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS="${GL_CFLAGS} ${CPPFLAGS}"
ax_save_LIBS="${LIBS}"

dnl
dnl First, check for the possibility that everything we need is already in
dnl GL_LIBS.
dnl
LIBS="${GL_LIBS} ${ax_save_LIBS}"
dnl
dnl libGLU typically links with libstdc++ on POSIX platforms.
dnl However, setting the language to C++ means that test program
dnl source is named "conftest.cc"; and Microsoft cl doesn't know what
dnl to do with such a file.
dnl
AC_LANG_PUSH([C++])
AS_IF([test X$ax_compiler_ms = Xyes],
      [AC_LANG_PUSH([C])])
AC_LINK_IFELSE(
[AX_CHECK_GLU_PROGRAM],
[ax_cv_check_glu_libglu=yes],
[LIBS=""
ax_check_libs="-lglu32 -lGLU"
for ax_lib in ${ax_check_libs}; do
  AS_IF([test X$ax_compiler_ms = Xyes],
        [ax_try_lib=`echo $ax_lib | sed -e 's/^-l//' -e 's/$/.lib/'`],
        [ax_try_lib="${ax_lib}"])
  LIBS="${ax_try_lib} ${GL_LIBS} ${ax_save_LIBS}"
  AC_LINK_IFELSE([AX_CHECK_GLU_PROGRAM],
                 [ax_cv_check_glu_libglu="${ax_try_lib}"; break])
done
])
AS_IF([test X$ax_compiler_ms = Xyes],
      [AC_LANG_POP([C])])
AC_LANG_POP([C++])

LIBS=${ax_save_LIBS}
CPPFLAGS=${ax_save_CPPFLAGS}])
AS_IF([test "X$ax_cv_check_glu_libglu" = Xno],
      [no_glu=yes; GLU_CFLAGS=""; GLU_LIBS=""],
      [AS_IF([test "X$ax_cv_check_glu_libglu" = Xyes],
             [GLU_LIBS="$GL_LIBS"],
             [GLU_LIBS="${ax_cv_check_glu_libglu} ${GL_LIBS}"])])
AC_SUBST([GLU_CFLAGS])
AC_SUBST([GLU_LIBS])

dnl
dnl Some versions of Mac OS X include a broken interpretation of the GLU
dnl tesselation callback function signature.
dnl
AS_IF([test "X$ax_cv_check_glu_libglu" != Xno],
[AC_CACHE_CHECK([for varargs GLU tesselator callback function type],
                [ax_cv_varargs_glu_tesscb],
[ax_cv_varargs_glu_tesscb=no
ax_save_CFLAGS="$CFLAGS"
CFLAGS="$GL_CFLAGS $CFLAGS"
AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([[
# ifdef HAVE_GL_GLU_H
#   include <GL/glu.h>
# else
#   include <OpenGL/glu.h>
# endif]],
                 [[GLvoid (*func)(...); gluTessCallback(0, 0, func)]])],
[ax_cv_varargs_glu_tesscb=yes])
CFLAGS="$ax_save_CFLAGS"])
AS_IF([test X$ax_cv_varargs_glu_tesscb = Xyes],
      [AC_DEFINE([HAVE_VARARGS_GLU_TESSCB], [1],
                 [Use nonstandard varargs form for the GLU tesselator callback])])])
])

dnl ---------------------------------------------------------------------------
dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/ax_check_glut.html
dnl ---------------------------------------------------------------------------

dnl
dnl SYNOPSIS
dnl
dnl   AX_CHECK_GLUT
dnl
dnl DESCRIPTION
dnl
dnl   Check for GLUT. If GLUT is found, the required compiler and linker flags
dnl   are included in the output variables "GLUT_CFLAGS" and "GLUT_LIBS",
dnl   respectively. If GLUT is not found, "no_glut" is set to "yes".
dnl
dnl   If the header "GL/glut.h" is found, "HAVE_GL_GLUT_H" is defined. If the
dnl   header "GLUT/glut.h" is found, HAVE_GLUT_GLUT_H is defined. These
dnl   preprocessor definitions may not be mutually exclusive.
dnl
dnl LICENSE
dnl
dnl   Copyright (c) 2009 Braden McDaniel <braden@endoframe.com>
dnl
dnl   This program is free software; you can redistribute it and/or modify it
dnl   under the terms of the GNU General Public License as published by the
dnl   Free Software Foundation; either version 2 of the License, or (at your
dnl   option) any later version.
dnl
dnl   This program is distributed in the hope that it will be useful, but
dnl   WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
dnl   Public License for more details.
dnl
dnl   You should have received a copy of the GNU General Public License along
dnl   with this program. If not, see <http://www.gnu.org/licenses/>.
dnl
dnl   As a special exception, the respective Autoconf Macro's copyright owner
dnl   gives unlimited permission to copy, distribute and modify the configure
dnl   scripts that are the output of Autoconf when processing the Macro. You
dnl   need not follow the terms of the GNU General Public License when using
dnl   or distributing such scripts, even though portions of the text of the
dnl   Macro appear in them. The GNU General Public License (GPL) does govern
dnl   all other use of the material that constitutes the Autoconf Macro.
dnl
dnl   This special exception to the GPL applies to versions of the Autoconf
dnl   Macro released by the Autoconf Archive. When you make and distribute a
dnl   modified version of the Autoconf Macro, you may extend this special
dnl   exception to the GPL to apply to your modified version as well.

AC_DEFUN([AX_CHECK_GLUT],
[AC_REQUIRE([AX_CHECK_GLU])dnl
AC_REQUIRE([AC_PATH_XTRA])dnl

ax_save_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS="${GLU_CFLAGS} ${CPPFLAGS}"
AC_CHECK_HEADERS([GL/glut.h GLUT/glut.h])
CPPFLAGS="${ax_save_CPPFLAGS}"

GLUT_CFLAGS=${GLU_CFLAGS}
GLUT_LIBS=${GLU_LIBS}

m4_define([AX_CHECK_GLUT_PROGRAM],
          [AC_LANG_PROGRAM([[
# if HAVE_WINDOWS_H && defined(_WIN32)
#   include <windows.h>
# endif
# ifdef HAVE_GL_GLUT_H
#   include <GL/glut.h>
# elif defined(HAVE_GLUT_GLUT_H)
#   include <GLUT/glut.h>
# else
#   error no glut.h
# endif]],
                           [[glutMainLoop()]])])

dnl
dnl If X is present, assume GLUT depends on it.
dnl
AS_IF([test X$no_x != Xyes],
      [GLUT_LIBS="${X_PRE_LIBS} -lXi ${X_EXTRA_LIBS} ${GLUT_LIBS}"])

AC_CACHE_CHECK([for GLUT library], [ax_cv_check_glut_libglut],
[ax_cv_check_glut_libglut="no"
AC_LANG_PUSH(C)
ax_save_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS="${GLUT_CFLAGS} ${CPPFLAGS}"
ax_save_LIBS="${LIBS}"
LIBS=""
ax_check_libs="-lglut32 -lglut"
for ax_lib in ${ax_check_libs}; do
  AS_IF([test X$ax_compiler_ms = Xyes],
        [ax_try_lib=`echo $ax_lib | sed -e 's/^-l//' -e 's/$/.lib/'`],
        [ax_try_lib="${ax_lib}"])
  LIBS="${ax_try_lib} ${GLUT_LIBS} ${ax_save_LIBS}"
  AC_LINK_IFELSE([AX_CHECK_GLUT_PROGRAM],
                 [ax_cv_check_glut_libglut="${ax_try_lib}"; break])
done

AS_IF([test "X$ax_cv_check_glut_libglut" = Xno -a "X$no_x" = Xyes],
[LIBS='-framework GLUT'
AC_LINK_IFELSE([AX_CHECK_GLUT_PROGRAM],
               [ax_cv_check_glut_libglut="$LIBS"])])

CPPFLAGS="${ax_save_CPPFLAGS}"
LIBS="${ax_save_LIBS}"
AC_LANG_POP(C)])

AS_IF([test "X$ax_cv_check_glut_libglut" = Xno],
      [no_glut="yes"; GLUT_CFLAGS=""; GLUT_LIBS=""],
      [GLUT_LIBS="${ax_cv_check_glut_libglut} ${GLUT_LIBS}"])

AC_SUBST([GLUT_CFLAGS])
AC_SUBST([GLUT_LIBS])
])dnl

dnl ---------------------------------------------------------------------------
dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/autoconf-archive/ax_pthread.html
dnl ---------------------------------------------------------------------------

dnl SYNOPSIS
dnl
dnl   AX_PTHREAD([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl DESCRIPTION
dnl
dnl   This macro figures out how to build C programs using POSIX threads. It
dnl   sets the PTHREAD_LIBS output variable to the threads library and linker
dnl   flags, and the PTHREAD_CFLAGS output variable to any special C compiler
dnl   flags that are needed. (The user can also force certain compiler
dnl   flags/libs to be tested by setting these environment variables.)
dnl
dnl   Also sets PTHREAD_CC to any special C compiler that is needed for
dnl   multi-threaded programs (defaults to the value of CC otherwise). (This
dnl   is necessary on AIX to use the special cc_r compiler alias.)
dnl
dnl   NOTE: You are assumed to not only compile your program with these flags,
dnl   but also link it with them as well. e.g. you should link with
dnl   $PTHREAD_CC $CFLAGS $PTHREAD_CFLAGS $LDFLAGS ... $PTHREAD_LIBS $LIBS
dnl
dnl   If you are only building threads programs, you may wish to use these
dnl   variables in your default LIBS, CFLAGS, and CC:
dnl
dnl     LIBS="$PTHREAD_LIBS $LIBS"
dnl     CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
dnl     CC="$PTHREAD_CC"
dnl
dnl   In addition, if the PTHREAD_CREATE_JOINABLE thread-attribute constant
dnl   has a nonstandard name, defines PTHREAD_CREATE_JOINABLE to that name
dnl   (e.g. PTHREAD_CREATE_UNDETACHED on AIX).
dnl
dnl   ACTION-IF-FOUND is a list of shell commands to run if a threads library
dnl   is found, and ACTION-IF-NOT-FOUND is a list of commands to run it if it
dnl   is not found. If ACTION-IF-FOUND is not specified, the default action
dnl   will define HAVE_PTHREAD.
dnl
dnl   Please let the authors know if this macro fails on any platform, or if
dnl   you have any other suggestions or comments. This macro was based on work
dnl   by SGJ on autoconf scripts for FFTW (http://www.fftw.org/) (with help
dnl   from M. Frigo), as well as ac_pthread and hb_pthread macros posted by
dnl   Alejandro Forero Cuervo to the autoconf macro repository. We are also
dnl   grateful for the helpful feedback of numerous users.
dnl
dnl LICENSE
dnl
dnl   Copyright (c) 2008 Steven G. Johnson <stevenj@alum.mit.edu>
dnl
dnl   This program is free software: you can redistribute it and/or modify it
dnl   under the terms of the GNU General Public License as published by the
dnl   Free Software Foundation, either version 3 of the License, or (at your
dnl   option) any later version.
dnl
dnl   This program is distributed in the hope that it will be useful, but
dnl   WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
dnl   Public License for more details.
dnl
dnl   You should have received a copy of the GNU General Public License along
dnl   with this program. If not, see <http://www.gnu.org/licenses/>.
dnl
dnl   As a special exception, the respective Autoconf Macro's copyright owner
dnl   gives unlimited permission to copy, distribute and modify the configure
dnl   scripts that are the output of Autoconf when processing the Macro. You
dnl   need not follow the terms of the GNU General Public License when using
dnl   or distributing such scripts, even though portions of the text of the
dnl   Macro appear in them. The GNU General Public License (GPL) does govern
dnl   all other use of the material that constitutes the Autoconf Macro.
dnl
dnl   This special exception to the GPL applies to versions of the Autoconf
dnl   Macro released by the Autoconf Archive. When you make and distribute a
dnl   modified version of the Autoconf Macro, you may extend this special
dnl   exception to the GPL to apply to your modified version as well.

AU_ALIAS([ACX_PTHREAD], [AX_PTHREAD])
AC_DEFUN([AX_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C
ax_pthread_ok=no

dnl We used to check for pthread.h first, but this fails if pthread.h
dnl requires special compiler flags (e.g. on True64 or Sequent).
dnl It gets checked for in the link test anyway.

dnl First of all, check if the user has set any of the PTHREAD_LIBS,
dnl etcetera environment variables, and if threads linking works using
dnl them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS" != x; then
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CFLAGS=$PTHREAD_CFLAGS])
        AC_TRY_LINK_FUNC(pthread_join, ax_pthread_ok=yes)
        AC_MSG_RESULT($ax_pthread_ok)
        if test x"$ax_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_CFLAGS=""
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
fi

dnl We must check for the threads library under a number of different
dnl names; the ordering is very important because some systems
dnl (e.g. DEC) have both -lpthread and -lpthreads, where one of the
dnl libraries is broken (non-POSIX).

dnl Create a list of thread flags to try.  Items starting with a "-" are
dnl C compiler flags, and other items are library names, except for "none"
dnl which indicates that we try without any flags at all, and "pthread-config"
dnl which is a program returning the flags for the Pth emulation library.

ax_pthread_flags="pthreads none -Kthread -kthread lthread -pthread -pthreads -mthreads pthread --thread-safe -mt pthread-config"

dnl The ordering *is* (sometimes) important.  Some notes on the
dnl individual items follow:

dnl pthreads: AIX (must check this before -lpthread)
dnl none: in case threads are in libc; should be tried before -Kthread and
dnl       other compiler flags to prevent continual compiler warnings
dnl -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
dnl -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
dnl lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
dnl -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
dnl -pthreads: Solaris/gcc
dnl -mthreads: Mingw32/gcc, Lynx/gcc
dnl -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
dnl      doesn't hurt to check since this sometimes defines pthreads too;
dnl      also defines -D_REENTRANT)
dnl      ... -mt is also the pthreads flag for HP/aCC
dnl pthread: Linux, etcetera
dnl --thread-safe: KAI C++
dnl pthread-config: use pthread-config program (for GNU Pth library)

case "${host_cpu}-${host_os}" in
        *solaris*)

        dnl On Solaris (at least, for some versions), libc contains stubbed
        dnl (non-functional) versions of the pthreads routines, so link-based
        dnl tests will erroneously succeed.  (We need to link with -pthreads/-mt/
        dnl -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        dnl a function called by this macro, so we could check for that, but
        dnl who knows whether they'll stub that too in a future libc.)  So,
        dnl we'll just look for -pthreads and -lpthread first:

        ax_pthread_flags="-pthreads pthread -mt -pthread $ax_pthread_flags"
        ;;

	*-darwin*)
	ax_pthread_flags="-pthread $ax_pthread_flags"
	;;
esac

if test x"$ax_pthread_ok" = xno; then
for flag in $ax_pthread_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;

                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_CFLAGS="$flag"
                ;;

		pthread-config)
		AC_CHECK_PROG(ax_pthread_config, pthread-config, yes, no)
		if test x"$ax_pthread_config" = xno; then continue; fi
		PTHREAD_CFLAGS="`pthread-config --cflags`"
		PTHREAD_LIBS="`pthread-config --ldflags` `pthread-config --libs`"
		;;

                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        save_CFLAGS="$CFLAGS"
        LIBS="$PTHREAD_LIBS $LIBS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        dnl Check for various functions.  We must include pthread.h,
        dnl since some functions may be macros.  (On the Sequent, we
        dnl need a special flag -Kthread to make this header compile.)
        dnl We check for pthread_join because it is in -lpthread on IRIX
        dnl while pthread_create is in libc.  We check for pthread_attr_init
        dnl due to DEC craziness with -lpthreads.  We check for
        dnl pthread_cleanup_push because it is one of the few pthread
        dnl functions on Solaris that doesn't have a non-functional libc stub.
        dnl We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>
	             static void routine(void* a) {a=0;}
	             static void* start_routine(void* a) {return a;}],
                    [pthread_t th; pthread_attr_t attr;
                     pthread_create(&th,0,start_routine,0);
                     pthread_join(th, 0);
                     pthread_attr_init(&attr);
                     pthread_cleanup_push(routine, 0);
                     pthread_cleanup_pop(0); ],
                    [ax_pthread_ok=yes])

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        AC_MSG_RESULT($ax_pthread_ok)
        if test "x$ax_pthread_ok" = xyes; then
                break;
        fi

        PTHREAD_LIBS=""
        PTHREAD_CFLAGS=""
done
fi

dnl Various other checks:
if test "x$ax_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        dnl Detect AIX lossage: JOINABLE attribute is called UNDETACHED.
	AC_MSG_CHECKING([for joinable pthread attribute])
	attr_name=unknown
	for attr in PTHREAD_CREATE_JOINABLE PTHREAD_CREATE_UNDETACHED; do
	    AC_TRY_LINK([#include <pthread.h>], [int attr=$attr; return attr;],
                        [attr_name=$attr; break])
	done
        AC_MSG_RESULT($attr_name)
        if test "$attr_name" != PTHREAD_CREATE_JOINABLE; then
            AC_DEFINE_UNQUOTED(PTHREAD_CREATE_JOINABLE, $attr_name,
                               [Define to necessary symbol if this constant
                                uses a non-standard name on your system.])
        fi

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
            *-aix* | *-freebsd* | *-darwin*) flag="-D_THREAD_SAFE";;
            *solaris* | *-osf* | *-hpux*) flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
            PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
        fi

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        dnl More AIX lossage: must compile with xlc_r or cc_r
	if test x"$GCC" != xyes; then
          AC_CHECK_PROGS(PTHREAD_CC, xlc_r cc_r, ${CC})
        else
          PTHREAD_CC=$CC
	fi
else
        PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_CC)

dnl Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$ax_pthread_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
        :
else
        ax_pthread_ok=no
        $2
fi
AC_LANG_RESTORE
])dnl AX_PTHREAD
