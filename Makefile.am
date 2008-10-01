## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src po

desktopdir = $(datadir)/applications
desktop_in_files = me-tv.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

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
	$(desktop_in_files) \
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

CLEANFILES = $(desktop_DATA)

DISTCLEANFILES = intltool-extract intltool-merge intltool-update
