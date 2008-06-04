## Process this file with automake to produce Makefile.in

## Created by Anjuta

gladedir = $(datadir)/me-tv/glade
glade_DATA = me-tv.glade \
	me-tv.xpm

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(ME_TV_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = me-tv

me_tv_SOURCES = \
	main.cc\
	../config.h \
	application.h \
	main_window.h \
	channels_dialog.h \
	preferences_dialog.h \
	scan_dialog.h

me_tv_LDFLAGS = 

me_tv_LDADD = $(ME_TV_LIBS)

EXTRA_DIST = $(glade_DATA)
