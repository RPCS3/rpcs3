#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#AC_PREREQ([2.63])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([BPMDetect.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

CFLAGS=
CPPFLAGS=
CXXFLAGS=
CCASFLAGS=

CFLAGS+=" -m32 "
CPPFLAGS+=" -m32 "
CXXFLAGS+=" -m32 "
CCASFLAGS+=" -m32 "

# Checks for header files.
AC_CHECK_HEADERS([limits.h memory.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_HEADER_STDBOOL

# Checks for library functions.
AC_CHECK_FUNCS([memmove memset])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
