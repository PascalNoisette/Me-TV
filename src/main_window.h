/*
 * Copyright (C) 2008 Michael Lamothe
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
	MetersDialog*							meters_dialog;
	Gtk::HScale*							h_scale_position;
	Gnome::UI::AppBar*						app_bar;
	DisplayMode								display_mode;
	Gtk::RadioButtonGroup					radio_button_group_devices;
	gboolean								initialise;
	guint									last_update_time;
	guint									last_poke_time;

	void stop();
	void fullscreen();
	void unfullscreen();
	gboolean is_fullscreen();
	void toggle_fullscreen();
	void set_display_mode(DisplayMode display_mode);
	void load_devices();
	void show_scheduled_recordings_dialog();
	void set_state(const Glib::ustring& name, gboolean state);
		
	bool on_motion_notify_event(GdkEventMotion* event);
	bool on_drawing_area_expose_event(GdkEventExpose* event);
	bool on_timeout();
	void on_error(const Glib::ustring& message);
	void on_menu_item_record_clicked();
	void on_menu_item_broadcast_clicked();
	void on_menu_item_quit_clicked();
	void on_menu_item_meters_clicked();
	void on_menu_item_schedule_clicked();
	void on_menu_item_channels_clicked();
	void on_menu_item_preferences_clicked();
	void on_menu_item_fullscreen_clicked();
	void on_menu_item_mute_clicked();
	void on_menu_item_help_contents_clicked();
	void on_menu_item_about_clicked();
	bool on_event_box_video_button_pressed(GdkEventButton* event);
	bool on_event_box_video_scroll_event(GdkEventScroll* event);
	void on_tool_button_record_clicked();
	void on_tool_button_mute_clicked();
	void on_tool_button_broadcast_clicked();
	void on_record_state_changed(gboolean record_state);
	void on_mute_state_changed(gboolean mute_state);
	void on_broadcast_state_changed(gboolean broadcast_state);

	void on_show();
	void on_hide();
public:
	MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade);
	~MainWindow();
		
	static MainWindow* create(Glib::RefPtr<Gnome::Glade::Xml> glade);
		
	gboolean is_recording();
	gboolean is_broadcasting();
	gboolean is_muted();

	Gtk::DrawingArea& get_drawing_area();
	void show_channels_dialog();
	void update();
};

#endif
