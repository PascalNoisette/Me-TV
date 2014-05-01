## Process this file with automake to produce Makefile.in

## Created by Anjuta

gladedir = $(datadir)/me-tv/glade
glade_DATA = me-tv.ui \
	me-tv.xpm

AM_CPPFLAGS = \
	$(ME_TV_CFLAGS) \
	$(WARN_CXXFLAGS) \
	-DPACKAGE_DATA_DIR=\""$(datadir)/me-tv"\"

AM_CXXFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = me-tv \
	me-tv-player \
	me-tv-player-xine

me_tv_player_SOURCES = \
	me-tv-player.cc

me_tv_player_LDADD =        \
	$(ME_TV_PLAYER_LIBS)

me_tv_player_xine_SOURCES = \
	me-tv-player-xine.cpp

me_tv_player_xine_LDADD =        \
	$(ME_TV_PLAYER_XINE_LIBS)

me_tv_SOURCES = \
	../config.h \
	main.cc\
	channel_stream.cc \
	channel_stream.h \
	frontend_thread.cc \
	frontend_thread.h \
	mpeg_stream.cc \
	mpeg_stream.h \
	buffer.cc \
	buffer.h \
	main_window.cc \
	application.h \
	main_window.h \
	channels_dialog.h \
	preferences_dialog.h \
	exception.h \
	thread.h \
	application.cc \
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
	channels_dialog.cc \
	thread.cc \
	gtk_epg_widget.cc \
	gtk_epg_widget.h \
	me-tv-ui.h \
	me-tv-ui.cc \
	device_manager.cc \
	engine.h \
	engine.cc \
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
	exception.cc \
	channels_conf_line.cc \
	channels_conf_line.h \
	initial_scan_line.cc \
	initial_scan_line.h \
	me-tv-i18n.h \
	epg_events.cc \
	epg_events.h \
	channel_manager.cc \
	channel_manager.h \
	scheduled_recording_manager.cc \
	scheduled_recording_manager.h \
	scan_dialog.cc \
	scan_dialog.h \
	crc32.cc \
	crc32.h \
	stream_manager.cc \
	stream_manager.h \
	epg_event_search_dialog.cc \
	epg_event_search_dialog.h \
	epg_event_dialog.cc \
	epg_event_dialog.h \
	edit_channel_dialog.cc \
	edit_channel_dialog.h \
	auto_record_dialog.cc \
	auto_record_dialog.h \
	configuration_manager.cc \
	configuration_manager.h

me_tv_LDFLAGS = \
	-Wl,--as-needed `getconf \
	LFS_LDFLAGS` `getconf \
	LFS64_LDFLAGS`

me_tv_LDADD = \
	$(ME_TV_LIBS)

EXTRA_DIST = $(glade_DATA)
