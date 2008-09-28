## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src

desktopdir = $(datadir)/applications
desktop_DATA = me-tv.desktop

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = \
	me-tv.png \
        src/me-tv.xpm \
        src/me-tv-recording.xpm

EXTRA_DIST = me-tv.1 \
	README\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS \
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

