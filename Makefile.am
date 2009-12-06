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

schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = me-tv.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

man_MANS = \
	me-tv.1 \
	me-tv-xine.1

EXTRA_DIST = \
	README\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS \
	me-tv.1 \
	me-tv-xine.1 \
	$(desktop_in_files) \
	$(schemas_in_files) \
	$(pixmaps_DATA) \
	intltool-update.in \
	intltool-merge.in \
	intltool-extract.in

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local: $(schemas_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schemas_DATA)
endif

# Copy all the spec files. Of course, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

CLEANFILES = $(desktop_DATA) $(schemas_DATA)

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

AM_CPPFLAGS = 
