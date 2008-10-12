## Process this file with automake to produce Makefile.in

## Created by Anjuta

gladedir = $(datadir)/me-tv/glade
glade_DATA = me-tv.glade \
	me-tv.xpm

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(ME_TV_CFLAGS) \
	$(SQLITE_CFLAGS) \
	$(WARN_CXXFLAGS) \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = me-tv

me_tv_SOURCES = \
	main.cc\
	../config.h \
	stream_thread.cc \
	stream_thread.h \
	application.h \
	main_window.h \
	channels_dialog.h \
	preferences_dialog.h \
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
	channels_dialog.cc \
	thread.cc \
	main_window.cc \
	gtk_epg_widget.cc \
	gtk_epg_widget.h \
	profile_manager.cc \
	me-tv-ui.h \
	me-tv-ui.cc \
	device_manager.cc \
	engine.h \
	meters_dialog.cc \
	engine.cc \
	profile.cc \
	profile.h \
	channel.h \
	epg_event.h \
	data.cc \
	data.h \
	epg_event.cc \
	me-tv.cc \
	channel.cc \
	scheduled_recording_dialog.cc \
	scheduled_recording_dialog.h \
	scheduled_recordings_dialog.cc \
	scheduled_recordings_dialog.h \
	preferences_dialog.cc \
	status_icon.cc \
	status_icon.h \
	scheduled_recording.h \
	scheduled_recording.cc \
	epg_thread.cc \
	epg_thread.h \
	xine_engine.cc \
	xine_engine.h \
	exception.cc \
	scan_window.cc \
	scan_window.h

me_tv_LDFLAGS = -Wl,--as-needed \
	`getconf LFS_LDFLAGS` \
        `getconf LFS64_LDFLAGS`

me_tv_LDADD = \
	$(ME_TV_LIBS) \
	$(SQLITE_LIBS)

EXTRA_DIST = $(glade_DATA)
