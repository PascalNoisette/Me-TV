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
	scan_dialog.h \
	exception.h \
	thread.h \
	application.cc \
	profile_manager.h \
	dvb_frontend.cc \
	dvb_frontend.h \
	dvb_scanner.cc \
	dvb_scanner.h \
	dvb_demuxer.cc \
	dvb_demuxer.h \
	dvb_si.cc \
	dvb_si.h \
	dvb_service.cc \
	dvb_service.h \
	dvb_transponder.cc \
	dvb_transponder.h \
	atsc_text.cc \
	atsc_text.h \
	device_manager.h \
	me-tv.h \
	meters_dialog.h \
	scan_dialog.cc \
	channels_dialog.cc \
	thread.cc \
	main_window.cc \
	channel_manager.h \
	gtk_epg_widget.cc \
	gtk_epg_widget.h \
	channel_manager.cc \
	profile_manager.cc \
	me-tv-ui.h \
	me-tv-ui.cc \
	device_manager.cc 

me_tv_LDFLAGS = 

me_tv_LDADD = $(ME_TV_LIBS)

EXTRA_DIST = $(glade_DATA)
