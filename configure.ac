dnl Autoconf settings for Melo-rad.io
AC_PREREQ([2.69])

dnl Initialize Autoconf for Melo-rad.io
AC_INIT([Melo-Rad.io], [0.0.1], [http://www.sparod.com/melo/], [melo-rad.io])

dnl Initialize Automake
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

dnl Set up macro directory
AC_CONFIG_MACRO_DIR([m4])

dnl Set source directory
AC_CONFIG_SRCDIR([src/melo_rad_io_plugin.c])

dnl Check for programs
AC_PROG_CC
AM_PROG_CC_C_O

dnl Check and init libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl Check for header files
AC_HEADER_STDC

dnl Check for Libmelo
LIBMELO_REQ=0.0.1
PKG_CHECK_MODULES([LIBMELO],
  libmelo >= $LIBMELO_REQ, [],
  [
    AC_MSG_ERROR(Libmelo $LIBMELO_REQ or later is required)
  ])

dnl Check for LibSoup
SOUP_REQ=2.40.0
PKG_CHECK_MODULES([LIBSOUP],
  libsoup-2.4 >= $SOUP_REQ, [],
  [
    AC_MSG_ERROR(LibSoup $SOUP_REQ or later is required)
  ])

dnl Set Melo libraries directory
melolibdir="\$(libdir)/melo"
AC_SUBST(melolibdir)

AC_CONFIG_FILES(
Makefile
src/Makefile
)
AC_OUTPUT
