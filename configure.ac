# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.69)
AC_INIT([Me TV], [3.0.0-SNAPSHOT], [http://launchpad.net/me-tv], me-tv)
AC_COPYRIGHT([Michael Lamothe <michael.lamothe@gmail.com>, Russel Winder <russel@winder.org.uk>])
AC_CONFIG_SRCDIR([src/main.cc])
AM_INIT_AUTOMAKE([1.14 foreign])
AC_GNU_SOURCE
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CXX
AC_ISC_POSIX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# GNOME aclocal, debug and maintainer flags
GNOME_COMMON_INIT
GNOME_DEBUG_CHECK
GNOME_MAINTAINER_MODE_DEFINES
GNOME_CXX_WARNINGS

if test x"$USE_MAINTAINER_MODE" != xno; then
	enable_debug=yes
fi

CXXFLAGS="$CXXFLAGS \
-std=c++1y \
-W \
-Wall \
-Wundef \
-Wcast-align \
-Wno-unused-parameter \
-Wextra \
-Wcast-align \
-Wcast-qual \
-Wcomment \
-Wformat \
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
IT_PROG_INTLTOOL([0.50.2])

# required versions
SQLITE_REQUIRED=3.8.4.3
GTKMM_REQUIRED=2.24.4
GIOMM_REQUIRED=2.38.2
GTHREAD_REQUIRED=2.38.2
GCONFMM_REQUIRED=2.28.0
X11_REQUIRED=1.6.1
DBUS_1_REQUIRED=1.6.12
DBUS_GLIB_1_REQUIRED=0.100.2
AC_SUBST(SQLITE_REQUIRED)
AC_SUBST(LIBGTKMM_REQUIRED)
AC_SUBST(GIOMM_REQUIRED)
AC_SUBST(GTHREAD_REQUIRED)
AC_SUBST(GCONFMM_REQUIRED)

PKG_CHECK_MODULES(ME_TV, [
        sqlite3 >= $SQLITE_REQUIRED
	gtkmm-2.4 >= $GTKMM_REQUIRED
	giomm-2.4 >= $GIOMM_REQUIRED
	gthread-2.0 >= $GTHREAD_REQUIRED
	gconfmm-2.6 >= $GCONFMM_REQUIRED
	x11 >= $X11_REQUIRED
	dbus-1 >= $DBUS_1_REQUIRED
        dbus-glib-1 >= $DBUS_GLIB_1_REQUIRED
])
AC_SUBST(ME_TV_CFLAGS)
AC_SUBST(ME_TV_LIBS)

LIBXINE_REQUIRED=1.2.5
AC_SUBST(LIBXINE_REQUIRED)

PKG_CHECK_MODULES(ME_TV_PLAYER_XINE, [
        libxine >= $LIBXINE_REQUIRED
        x11 >= $X11_REQUIRED
        giomm-2.4 >= $GIOMM_REQUIRED
])

# GConf
AC_PATH_PROG(GCONFTOOL, gconftool-2, no)

if test x"$GCONFTOOL" = xno; then
  AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
fi

AM_GCONF_SOURCE_2

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
	C++ Compiler ...................: ${CXX}
	C++ Compiler flags .............: ${CXXFLAGS}
	Enable compile warnings ....: ${enable_cxx_warnings}
	Enable more warnings .......: ${enable_more_warnings}
	Extra Compiler Warnings ....: ${WARN_CXXFLAGS}
	Debug support ..............: ${enable_debug}
	Installing into prefix .....: ${prefix}

Type make to build ${PACKAGE_STRING}

"
