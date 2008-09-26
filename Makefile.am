## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src po

desktopdir = $(datadir)/applications
desktop_DATA = me-tv.desktop

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = \
	me-tv.png \
        src/me-tv.xpm \
        src/me-tv-recording.xpm

me_tvdocdir = ${prefix}/doc/me-tv
me_tvdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS

EXTRA_DIST = me-tv.1 \
	$(me_tvdoc_DATA) \
	$(desktop_DATA) \
	$(pixmaps_DATA) \
	intltool-update.in \
	intltool-merge.in \
	intltool-extract.in

# Copy all the spec files. Of course, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

