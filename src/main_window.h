/*
 * Copyright (C) 2010 Michael Lamothe
 *
 * This file is part of Me TV
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include "me-tv.h"
#include "gtk_epg_widget.h"
#include "engine.h"
#include <dbus/dbus.h>
#include <gtkmm/volumebutton.h>

typedef enum
{
	VIEW_MODE_VIDEO,
	VIEW_MODE_CONTROLS
} ViewMode;

class MainWindow : public Gtk::Window
{
private:
	const Glib::RefPtr<Gtk::Builder>		builder;
	Gtk::DrawingArea*						drawing_area_video;
	GtkEpgWidget*							widget_epg;
	guint									last_motion_time;
	GdkCursor*								hidden_cursor;
	gboolean								is_cursor_visible;
	Gtk::HScale*							h_scale_position;
	Gtk::Statusbar*							statusbar;
	Gtk::VolumeButton*						volume_button;
	ViewMode								view_mode;
	ViewMode								prefullscreen_view_mode;
	guint									last_update_time;
	guint									timeout_source;
	Engine*									engine;
	gint									output_fd;
	Glib::StaticRecMutex					mutex;
	gboolean								mute_state;
	gboolean								maximise_forced;
	guint									channel_change_timeout;
	guint									temp_channel_number;
	sigc::connection						connection_exception;
	guint									screensaver_inhibit_cookie;
	
	void stop();
	void set_view_mode(ViewMode display_mode);
	void load_devices();
	void set_state(const Glib::ustring& name, gboolean state);
	void add_channel_number(guint channel_number);
	void toggle_mute();
	void set_mute_state(gboolean state);
	
	bool on_delete_event(GdkEventAny* event);
	bool on_motion_notify_event(GdkEventMotion* event);
	bool on_drawing_area_expose_event(GdkEventExpose* event);
	static gboolean on_timeout(gpointer data);
	void on_timeout();
	bool on_key_press_event(GdkEventKey* event);
	bool on_event_box_video_button_pressed(GdkEventButton* event);
	void on_menu_item_audio_stream_activate(guint audio_stream_index);
	void on_menu_item_subtitle_stream_activate(guint audio_stream_index);

	void on_show();
	void on_hide();
			
	void on_next_channel();
	void on_previous_channel();
	void on_change_view_mode();
	void on_devices();
	void on_channels();
	void on_scheduled_recordings();
	void on_epg_event_search();
	void on_meters();
	void on_preferences();
	void on_fullscreen();
	void on_mute();
	void on_increase_volume();
	void on_decrease_volume();
	void on_button_volume_value_changed(double value);
	void on_audio_channel_both();
	void on_audio_channel_left();
	void on_audio_channel_right();
	void on_about();

	void create_engine();
public:
	MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
	virtual ~MainWindow();
		
	static MainWindow* create(Glib::RefPtr<Gtk::Builder> builder);
		
	void on_exception();

	void show_channels_dialog();
	void show_preferences_dialog();
	void show_scheduled_recordings_dialog();
	void show_epg_event_search_dialog();
	void show_error_dialog(const Glib::ustring& message);
	
	void toggle_visibility();
	void update();
	void save_geometry();

	void play(const Glib::ustring& mrl);
	void pause(gboolean state);
	void restart_engine();
	void start_engine();
	void stop_engine();

	void fullscreen(gboolean change_mode = true);
	void unfullscreen(gboolean restore_mode = true);
	gboolean is_fullscreen();

	gboolean is_screensaver_inhibited() { return (screensaver_inhibit_cookie != 0); }
	void inhibit_screensaver(gboolean activate);
};

#endif
