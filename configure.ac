#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# Prelude.
AC_PREREQ([2.59])
AC_INIT(OOmnik, 0.1.1, http://www.oomnik.ru)
AC_CONFIG_HEADERS([config.h])

# place to put some extra build scripts installed
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.9 foreign])

OOMNIK_MAJOR_VERSION=0
OOMNIK_MINOR_VERSION=1
OOMNIK_MICRO_VERSION=1
OOMNIK_VERSION=$OOMNIK_MAJOR_VERSION.$OOMNIK_MINOR_VERSION.$OOMNIK_MICRO_VERSION

AC_SUBST(OOMNIK_MAJOR_VERSION)
AC_SUBST(OOMNIK_MINOR_VERSION)
AC_SUBST(OOMNIK_MICRO_VERSION)
AC_SUBST(OOMNIK_VERSION)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL

CFLAGS=-I/usr/include/libxml2

if test -n "$GCC"; then
  CFLAGS="$CFLAGS -Wall -g -O0 -Wmissing-prototypes -Wwrite-strings"
fi

# Checks for libraries.
DB4_LIBS="-ldb"
AC_SUBST(DB4_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

AC_CONFIG_FILES([oomnik.pc
                 Makefile
                 src/Makefile])

AC_OUTPUT
