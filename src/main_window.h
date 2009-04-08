/*
 * Copyright (C) 2009 Michael Lamothe
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
#include "meters_dialog.h"
#include "engine.h"

typedef enum
{
	DISPLAY_MODE_VIDEO,
	DISPLAY_MODE_CONTROLS,
	DISPLAY_MODE_EPG
} DisplayMode;

class MainWindow : public Gnome::UI::App
{
private:
	const Glib::RefPtr<Gnome::Glade::Xml>	glade;
	Gtk::DrawingArea*						drawing_area_video;
	GtkEpgWidget*							widget_epg;
	guint									last_motion_time;
	GdkCursor*								hidden_cursor;
	gboolean								is_cursor_visible;
	Gtk::HScale*							h_scale_position;
	Gnome::UI::AppBar*						app_bar;
	DisplayMode								display_mode, prefullscreen;
	guint									last_update_time;
	guint									last_poke_time;
	guint									timeout_source;
	Gtk::Menu								audio_streams_menu;
	Engine*									engine;
	gint									output_fd;
	Glib::StaticRecMutex					mutex;
	gboolean								mute_state;
	Engine::AudioChannelState				audio_channel_state;
	guint									audio_stream_index;

	void stop();
	void fullscreen();
	void unfullscreen();
	gboolean is_fullscreen();
	void toggle_fullscreen();
	void toggle_mute();
	void set_mute_state(gboolean mute_state);
	void set_display_mode(DisplayMode display_mode);
	void load_devices();
	void show_scheduled_recordings_dialog();
	void set_state(const Glib::ustring& name, gboolean state);
		
	bool on_delete_event(GdkEventAny* event);
	bool on_motion_notify_event(GdkEventMotion* event);
	bool on_drawing_area_expose_event(GdkEventExpose* event);
	static gboolean on_timeout(gpointer data);
	void on_timeout();
	bool on_key_press_event(GdkEventKey* event);
	void on_menu_item_record_clicked();
	void on_menu_item_broadcast_clicked();
	void on_menu_item_quit_clicked();
	void on_menu_item_meters_clicked();
	void on_menu_item_schedule_clicked();
	void on_menu_item_channels_clicked();
	void on_menu_item_devices_clicked();
	void on_menu_item_preferences_clicked();
	void on_menu_item_fullscreen_clicked();
	void on_menu_item_mute_clicked();
	void on_menu_item_help_contents_clicked();
	void on_menu_item_about_clicked();
	bool on_event_box_video_button_pressed(GdkEventButton* event);
	void on_tool_button_record_clicked();
	void on_tool_button_mute_clicked();
	void on_tool_button_broadcast_clicked();
	void on_menu_item_audio_stream_activate(guint audio_stream_index);
	void on_radio_menu_item_audio_channels_both();
	void on_radio_menu_item_audio_channels_left();
	void on_radio_menu_item_audio_channels_right();

	void on_show();
	void on_hide();
	
	void set_next_display_mode();
		
	void create_engine();
public:
	MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade);
	virtual ~MainWindow();
		
	static MainWindow* create(Glib::RefPtr<Gnome::Glade::Xml> glade);
		
	void show_devices_dialog();
	void show_channels_dialog();
	void show_preferences_dialog();
	void toggle_visibility();
	void update();
	void save_geometry();

	void play(const Glib::ustring& mrl);
	void start_engine();
	void stop_engine();
};

#endif
