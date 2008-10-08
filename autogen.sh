#!/bin/bash
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="Me TV"
REQUIRED_AUTOCONF_VERSION=2.53
REQUIRED_AUTOMAKE_VERSION=1.9

(test -f $srcdir/configure.ac \
  && test -d $srcdir/src \
  && test -f $srcdir/src/main.cc ) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

gnome_autogen=`which gnome-autogen.sh`
[ 0 = $? ] || {
    echo "You need to install gnome-common from the GNOME"
    exit 1
}

#USE_COMMON_DOC_BUILD=yes
USE_GNOME2_MACROS=1 . "$gnome_autogen"
