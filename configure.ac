# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT([Me TV], [0.9.1], [http://launchpad.net/me-tv], me-tv)
AC_COPYRIGHT([Michael Lamothe <michael.lamothe@gmail.com>])
AC_CONFIG_SRCDIR([src/main.cc])
AM_INIT_AUTOMAKE([1.9 foreign])
AC_GNU_SOURCE
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# GNOME aclocal, debug and maintainer flags
GNOME_COMMON_INIT
# debug, warnings, deprecated
if test x"$enable_more_warnings" = xyes ; then
	enable_cxx_warnings=yes
fi
if test x"$USE_MAINTAINER_MODE" != xno; then
	enable_debug=yes
	CFLAGS=""
fi
if test x"$enable_debug" = xyes ; then
#	enable_cxx_warnings=yes
	enable_more_warnings=yes
	CFLAGS="$CFLAGS -O0 -g"
fi
GNOME_DEBUG_CHECK
GNOME_MAINTAINER_MODE_DEFINES
GNOME_CXX_WARNINGS
AC_ARG_ENABLE(more-warnings,
   AC_HELP_STRING([--enable-more-warnings],
                  [enable more compiler warnings (default=no)]),
                  [enable_more_warnings="$enableval"], [enable_more_warnings=no])
AC_MSG_CHECKING([what more warning flags to pass to the C compiler])
if test x"$enable_more_warnings" != xno ; then
	more_options=" \
-Wextra \
-Wcast-align \
-Wcast-qual \
-Wcomment \
-Wformat \
-Wimplicit \
-Wmissing-braces \
-Wpacked \
-Wparentheses \
-Wpointer-arith \
-Wreturn-type \
-Wsequence-point \
-Wstrict-aliasing \
-Wstrict-aliasing=2 \
-Wswitch-default \
-Wundef \
"
	previous_CXXFLAGS="$CXXFLAGS"
	CXXFLAGS="${WARN_CXXFLAGS}"
	for option in ${more_options} ; do
		SAVE_CXXFLAGS="$CXXFLAGS"
		CXXFLAGS="$CXXFLAGS $option"
		AC_TRY_COMPILE([], [],
			has_option=yes,
			has_option=no,)
		if test $has_option = no; then
			CXXFLAGS="$SAVE_CXXFLAGS"
		fi
		unset has_option
		unset SAVE_CXXFLAGS
	done
	unset option more_options
	WARN_CXXFLAGS="$CXXFLAGS"
	CXXFLAGS="${previous_CXXFLAGS}"
	unset previous_CXXFLAGS
	AC_MSG_RESULT([$WARN_CXXFLAGS])
else
	AC_MSG_RESULT(none)
fi

# Checks for header files.
AC_PATH_X
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h sys/ioctl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([localtime_r mempcpy memset mkfifo select strerror tzset])

# Checks for libraries.

# gettext support
GETTEXT_PACKAGE=${PACKAGE}
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [gettext domain string])
AM_GLIB_GNU_GETTEXT
AM_GLIB_DEFINE_LOCALEDIR(PACKAGE_LOCALE_DIR)
IT_PROG_INTLTOOL([0.35.0])

# required versions
SQLITE_REQUIRED=3.0
LIBGNOMEMM_REQUIRED=2.20.0
LIBGNOMEUI_REQUIRED=2.20
GIOMM_REQUIRED=2.10.0
GCONF_REQUIRED=2.0
GTHREAD_REQUIRED=2.10.0
GNET_REQUIRED=2.0.0
LIBXINE_REQUIRED=1.1.7
LIBVLC_REQUIRED=0.9
LIBGSTREAMER_REQUIRED=0.10
AC_SUBST(SQLITE_REQUIRED)
AC_SUBST(LIBGNOMEMM_REQUIRED)
AC_SUBST(LIBGNOMEUI_REQUIRED)
AC_SUBST(GIOMM_REQUIRED)
AC_SUBST(GCONF_REQUIRED)
AC_SUBST(GTHREAD_REQUIRED)
AC_SUBST(LIBXTST_REQUIRED)
AC_SUBST(GNET_REQUIRED)
AC_SUBST(LIBXINE_REQUIRED)
AC_SUBST(LIBVLC_REQUIRED)
AC_SUBST(LIBGSTREAMER_REQUIRED)

# check for sqlite
PKG_CHECK_MODULES(SQLITE, [sqlite3 >= $SQLITE_REQUIRED])

# check for additional requirements when optional engines are enabled
# default engine is xine, other engines are disabled at default
# we are only checking optional engines
AC_ARG_ENABLE(xine-engine,
        AC_HELP_STRING([--enable-xine-engine],[Enable Xine engine support (default disabled)]),
        enable_xine_engine="$enableval", enable_xine_engine=yes)
if test x"$enable_xine_engine" = "xyes" ; then
# the binary path test is not required in the build time
# packagers should notice that the MPlayer binary is required in the install time
        AC_DEFINE(ENABLE_XINE_ENGINE,1,[Define to 1 if Xine engine is used.])
fi

AC_ARG_ENABLE(mplayer-engine, 
	AC_HELP_STRING([--enable-mplayer-engine],[Enable experimental MPlayer engine support (default disabled)]), 
	enable_mplayer_engine="$enableval", enable_mplayer_engine=no)
if test x"$enable_mplayer_engine" = "xyes" ; then
# the binary path test is not required in the build time
# packagers should notice that the MPlayer binary is required in the install time
	AC_DEFINE(ENABLE_MPLAYER_ENGINE,1,[Define to 1 if MPlayer engine is used.])
fi

AC_ARG_ENABLE(libvlc-engine, 
	AC_HELP_STRING([--enable-libvlc-engine],[Enable experimental VLC library engine support (default disabled)]), 
	enable_libvlc_engine="$enableval", enable_libvlc_engine=no)
if test x$enable_libvlc_engine = xyes; then
PKG_CHECK_MODULES([LIBVLC], libvlc >= $LIBVLC_REQUIRED, found_libvlc=yes, found_libvlc=no)
	if test x"$found_libvlc" = "xyes" ; then
		AC_DEFINE(ENABLE_LIBVLC_ENGINE,1,[Define to 1 if libvlc is available and VLC engine is used.])
		AC_SUBST(LIBVLC_CFLAGS)
		AC_SUBST(LIBVLC_LIBS)
	else
		AC_MSG_ERROR([Cannot find required vlc development package installed])
	fi
fi

AC_ARG_ENABLE(xine-lib-engine, 
	AC_HELP_STRING([--enable-xine-lib-engine],[Enable experimental xine-lib engine support (default disabled)]), 
	enable_xine_lib_engine="$enableval", enable_xine_lib_engine=no)
if test x$enable_xine_lib_engine = xyes; then
PKG_CHECK_MODULES([XINE_LIB], libxine >= $LIBXINE_REQUIRED, found_xine_lib=yes, found_xine_lib=no)
	if test x"$found_xine_lib" = "xyes" ; then
		AC_DEFINE(ENABLE_XINE_LIB_ENGINE,1,[Define to 1 if xine-lib is available and xine-lib engine is used.])
		AC_SUBST(XINE_LIB_CFLAGS)
		AC_SUBST(XINE_LIB_LIBS)
	else
		AC_MSG_ERROR([Cannot find required xine-lib development package installed])
	fi
fi

AC_ARG_ENABLE(libgstreamer-engine, 
	AC_HELP_STRING([--enable-libgstreamer-engine],[Enable experimental GStreamer library engine support (default enabled)]), 
	enable_libgstreamer_engine="$enableval", enable_libgstreamer_engine=no)
if test x$enable_libgstreamer_engine = xyes; then
PKG_CHECK_MODULES([LIBGSTREAMER], gstreamer-0.10 >= $LIBGSTREAMER_REQUIRED gstreamer-interfaces-0.10 >= $LIBGSTREAMER_REQUIRED, found_libgstreamer=yes, found_libgstreamer=no)
	if test x"$found_libgstreamer" = "xyes" ; then
		AC_DEFINE(ENABLE_LIBGSTREAMER_ENGINE,1,[Define to 1 if GStreamer library is available and GStreamer engine is used.])
		AC_SUBST(LIBGSTREAMER_CFLAGS)
		AC_SUBST(LIBGSTREAMER_LIBS)
	else
		AC_MSG_ERROR([Cannot find required GStreamer development package installed])
	fi
fi

PKG_CHECK_MODULES(ME_TV, [
libgnomemm-2.6 >= $LIBGNOMEMM_REQUIRED libgnomeuimm-2.6 >= $LIBGNOMEUI_REQUIRED giomm-2.4 >= $GIOMM_REQUIRED gthread-2.0 >= $GTHREAD_REQUIRED gconfmm-2.6 >= $GCONF_REQUIRED gnet-2.0 >= $GNET_REQUIRED])
AC_SUBST(ME_TV_CFLAGS)
AC_SUBST(ME_TV_LIBS)

##################################################
# Check for gtk-doc.
##################################################
AC_ARG_WITH(html-dir, [  --with-html-dir=PATH path to installed docs ])
if test "x$with_html_dir" = "x" ; then
  HTML_DIR='${datadir}/gtk-doc/html'
else
  HTML_DIR=$with_html_dir
fi
AC_SUBST(HTML_DIR)

gtk_doc_min_version=1.0
AC_MSG_CHECKING([gtk-doc version >= $gtk_doc_min_version])
if pkg-config --atleast-version=$gtk_doc_min_version gtk-doc; then
  AC_MSG_RESULT(yes)
  GTKDOC=true
else
  AC_MSG_RESULT(no)
  GTKDOC=false
fi
dnl Let people disable the gtk-doc stuff.
AC_ARG_ENABLE(gtk-doc,
              [  --enable-gtk-doc  Use gtk-doc to build documentation [default=auto]],
	      enable_gtk_doc="$enableval", enable_gtk_doc=auto)
if test x$enable_gtk_doc = xauto ; then
  if test x$GTKDOC = xtrue ; then
    enable_gtk_doc=yes
  else
    enable_gtk_doc=no
  fi
fi
AM_CONDITIONAL(ENABLE_GTK_DOC, test x$enable_gtk_doc = xyes)

AC_CONFIG_FILES([
Makefile
src/Makefile
po/Makefile.in
])
AC_CONFIG_HEADERS([config.h:config.h.in])
AC_OUTPUT


dnl Print a little note
echo "

Configure summary:

	Source code location .......: ${srcdir}
	Compiler ...................: ${CC}
	Compiler flags .............: ${CFLAGS}
	Enable compile warnings ....: ${enable_cxx_warnings}
	Enable more warnings .......: ${enable_more_warnings}
	Extra Compiler Warnings ....: ${WARN_CXXFLAGS}
	Debug support ..............: ${enable_debug}
	Installing into prefix .....: ${prefix}

Optional engines support:
	xine engine ................: ${enable_xine_engine}
	libvlc engine ..............: ${enable_libvlc_engine}
	MPlayer engine .............: ${enable_mplayer_engine}
	xine-lib engine ............: ${enable_xine_lib_engine}
	GStreamer library engine ...: ${enable_libgstreamer_engine}

Type make to build ${PACKAGE_STRING}

"
