dnl Process this file with autoconf to create configure.

AC_PREREQ([2.57])
AC_INIT(vtwm,[5.5.0], [mailto:vtwm-hackers@lists.sandelman.ca],vtwm)
AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE
AC_PREFIX_DEFAULT([/usr])

AM_CONFIG_HEADER(config.h)

AC_PATH_PROG(LN, ln)
AC_PATH_PROG(CP, cp)
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_YACC
AC_PROG_LEX

AC_CHECK_FUNC([mkstemp],
   AC_DEFINE(HAS_MKSTEMP,1,[Define to 1 if you have the `mkstemp' function.]))

AC_CHECK_LIB(rplay,replay_destroy)
AC_CHECK_LIB(rplay,replay_create,SOUND_SUPPORT=,SOUND_SUPPORT=-DNO_SOUND_SUPPORT)
AC_SUBST(SOUND_SUPPORT)

dnl Checks for pkg-config packages
PKG_CHECK_MODULES(VTWM, x11 xext xt xmu)

dnl Font height dependent text layout spacing
AC_ARG_ENABLE(nexpm,AC_HELP_STRING([--disable-nexpm],[Turn off nexpm building]),[if test "$enable_nexpm" = "yes"; then ENABLE_NEXPM=yes; fi ],[ENABLE_NEXPM=yes])

dnl xpm library checks
PKG_CHECK_EXISTS(xpm,[PKG_CHECK_MODULES(xpm, xpm)],[xpm_CFLAGS=-DNO_XPM_SUPPORT; unset ENABLE_NEXPM])
AC_SUBST(xpm_CFLAGS)
AC_SUBST(xpm_LIBS)
AC_SUBST(ENABLE_NEXPM)
AM_CONDITIONAL(ENABLE_NEXPM, test x$ENABLE_NEXPM = xyes)

dnl png library checks
PKG_CHECK_EXISTS(libpng,[PKG_CHECK_MODULES(png, libpng)],[png_CFLAGS=-DNO_PNG_SUPPORT])
AC_SUBST(png_CFLAGS)
AC_SUBST(png_LIBS)

dnl regexp library checks
AC_SEARCH_LIBS(regcomp,regex c,[regex_LIBS=$LIBS;regex_CFLAGS=],[regex_CFLAGS=-DNO_REGEX_SUPPORT])
AC_SUBST(regex_CFLAGS)
AC_SUBST(regex_LIBS)

dnl Font height dependent text layout spacing
AC_ARG_ENABLE(spacing,AC_HELP_STRING([--disable-spacing],[Turn off font height dependent text layout spacing]),[if test "$enable_spacing" = "yes"; then TWM_USE_SPACING=-DTWM_USE_SPACING; fi ],[TWM_USE_SPACING=-DTWM_USE_SPACING])
AC_SUBST(TWM_USE_SPACING)

dnl Opacity
AC_ARG_ENABLE(opacity,AC_HELP_STRING([--disable-opacity],[Turn off _NET_WM_WINDOW_OPACITY client window property support]),[if test "$enable_opacity" = "yes"; then TWM_USE_OPACITY=-DTWM_USE_OPACITY; fi ],[TWM_USE_OPACITY=-DTWM_USE_OPACITY])
AC_SUBST(TWM_USE_OPACITY)

dnl Sloppy focus
AC_ARG_ENABLE(sloppyfocus,AC_HELP_STRING([--disable-sloppyfocus],[Turn off sloppy window focus]),[if test "$enable_sloppyfocus" = "yes"; then TWM_USE_SLOPPYFOCUS=-DTWM_USE_SLOPPYFOCUS; fi ],[TWM_USE_SLOPPYFOCUS=-DTWM_USE_SLOPPYFOCUS])
AC_SUBST(TWM_USE_SLOPPYFOCUS)

dnl Version window build information
AC_ARG_ENABLE(buildinfo,AC_HELP_STRING([--disable-build-info],[Prevent version window from having build information]),[if test "$enable_build_info" = "no"; then NO_BUILD_INFO=-DNO_BUILD_INFO; fi ])
AC_SUBST(NO_BUILD_INFO)

dnl xft library checks
PKG_CHECK_EXISTS(xft,[PKG_CHECK_MODULES(xft, xft);xft_AUX_CFLAGS=-DTWM_USE_XFT])
AC_SUBST(xft_CFLAGS)
AC_SUBST(xft_AUX_CFLAGS)
AC_SUBST(xft_LIBS)

dnl xrandr library checks
PKG_CHECK_EXISTS(xrandr,[PKG_CHECK_MODULES(xrandr, xrandr);xrandr_AUX_CFLAGS=-DTWM_USE_XRANDR])
AC_SUBST(xrandr_CFLAGS)
AC_SUBST(xrandr_AUX_CFLAGS)
AC_SUBST(xrandr_LIBS)

dnl xinerama library checks
PKG_CHECK_EXISTS(xinerama,[PKG_CHECK_MODULES(xinerama, xinerama);xinerama_AUX_CFLAGS=-DTWM_USE_XINERAMA])
AC_SUBST(xinerama_CFLAGS)
AC_SUBST(xinerama_AUX_CFLAGS)
AC_SUBST(xinerama_LIBS)

AC_CHECK_HEADER(sys/select.h,NEED_SELECT_H=-DNEED_SELECT_H)
AC_SUBST(NEED_SELECT_H)
AC_CHECK_HEADER(process.h,NEED_PROCESS_H=-DNEED_PROCESS_H)
AC_SUBST(NEED_PROCESS_H)

AC_CHECK_FUNC(putenv,,NEED_PUTENV_F=-DNEED_PUTENV_F)
AC_SUBST(NEED_PUTENV_F)

dnl M4
AC_ARG_ENABLE(m4,AC_HELP_STRING([--disable-m4],[Turn off m4 vtwmrc processing]),[if test "$enable_m4" = "no"; then NO_M4_SUPPORT=-DNO_M4_SUPPORT; fi ])
AC_PATH_PROG(M4_BIN, m4)
if test -z "$M4_BIN"; then NO_M4_SUPPORT=-DNO_M4_SUPPORT; fi
AC_SUBST(NO_M4_SUPPORT)

PKG_CHECK_EXISTS(xpm)
AC_SUBST(VTWM_CFLAGS)
AC_SUBST(VTWM_LIBS)

XORG_MANPAGE_SECTIONS
XORG_RELEASE_VERSION

AC_ARG_WITH([SYS_VTWMRC_LOOK], AC_HELP_STRING([--disable-3dlook],['2D' or '3D' (3D default) look and feel]), [SYS_VTWMRC_LOOK=$withval],[SYS_VTWMRC_LOOK=3D])
AC_SUBST(SYS_VTWMRC_LOOK)


AC_OUTPUT([Makefile])
