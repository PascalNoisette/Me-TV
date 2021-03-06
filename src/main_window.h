/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright (C) 2011 Michael Lamothe
 *  Copyright © 2014  Russel Winder
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include "me-tv.h"
#include "gtk_epg_widget.h"
#include "engine.h"
#include <dbus/dbus.h>
#include <gtkmm/volumebutton.h>

typedef enum {
	VIEW_MODE_VIDEO,
	VIEW_MODE_CONTROLS
} ViewMode;

class MainWindow: public Gtk::Window {
private:
	const Glib::RefPtr<Gtk::Builder> builder;
	Gtk::DrawingArea * drawing_area_video;
	GtkEpgWidget * widget_epg;
	guint last_motion_time;
	GdkCursor * hidden_cursor;
	gboolean is_cursor_visible;
	Gtk::MenuBar * menu_bar;
	Gtk::VolumeButton * volume_button;
	Gtk::HBox * hbox_controls;
	Gtk::Label * label_time;
	ViewMode view_mode;
	ViewMode prefullscreen_view_mode;
	guint last_update_time;
	guint timeout_source;
	Engine * engine;
	gint output_fd;
	Glib::Threads::RecMutex mutex;
	gboolean mute_state;
	guint channel_change_timeout;
	guint temp_channel_number;
	void stop();
	void set_view_mode(ViewMode display_mode);
	void load_devices();
	void set_state(Glib::ustring const & name, gboolean state);
	void add_channel_number(guint channel_number);
	void toggle_mute();
	void set_mute_state(gboolean state);
	void set_status_text(Glib::ustring const & text);
	void select_channel_to_play();
	void play(Glib::ustring const & mrl);
	void pause(gboolean state);
	void restart_engine();
	void start_engine();
	void stop_engine();
	void toggle_visibility();
	void save_geometry();
	void fullscreen(gboolean change_mode = true);
	void unfullscreen(gboolean restore_mode = true);
	gboolean is_fullscreen();
	bool on_delete_event(GdkEventAny * event);
	bool on_motion_notify_event(GdkEventMotion * event);
	bool on_drawing_area_expose_event(GdkEventExpose * event);
	static gboolean on_timeout(gpointer data);
	void on_timeout();
	bool on_key_press_event(GdkEventKey * event);
	bool on_event_box_video_button_pressed(GdkEventButton * event);
	void on_menu_item_audio_stream_activate(guint audio_stream_index);
	void on_menu_item_subtitle_stream_activate(guint audio_stream_index);
	void on_start_display(guint channel_id);
	void on_stop_display();
	void on_update();
	void on_error(Glib::ustring const & message);
	void on_show();
	void on_hide();
	void on_about();
	void on_auto_record();
	void on_audio_channel_both();
	void on_audio_channel_left();
	void on_audio_channel_right();
	void on_button_volume_value_changed(double value);
	void on_change_view_mode();
	void on_channels();
	void on_decrease_volume();
	void on_devices();
	void on_epg_event_search();
	void on_fullscreen();
	void on_increase_volume();
	void on_meters();
	void on_mute();
	void on_next_channel();
	void on_preferences();
	void on_previous_channel();
	void on_present();
	void on_scheduled_recordings();
	void create_engine();
public:
	MainWindow(BaseObjectType * cobject, Glib::RefPtr<Gtk::Builder> const & builder);
	virtual ~MainWindow();
	static MainWindow * create(Glib::RefPtr<Gtk::Builder> builder);
	void on_exception();
	void show_channels_dialog();
	void show_preferences_dialog();
	void show_scheduled_recordings_dialog();
	void show_epg_event_search_dialog();
	void show_error(Glib::ustring const & message);
};

#endif
