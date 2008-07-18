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

typedef enum DisplayMode
{
	DISPLAY_MODE_VIDEO,
	DISPLAY_MODE_EPG
};

class MainWindow : public Gtk::Window
{
private:
	const Glib::RefPtr<Gnome::Glade::Xml>&	glade;
	Gtk::DrawingArea*						drawing_area_video;
	GtkEpgWidget*							widget_epg;
	guint									last_motion_time;
	GdkCursor*								hidden_cursor;
	gboolean								is_cursor_visible;
	MetersDialog*							meters_dialog;
	Gtk::HScale*							h_scale_position;
	Gtk::Statusbar*							statusbar;
	DisplayMode								display_mode;
	Gtk::RadioButtonGroup					radio_button_group_devices;

	void stop();
	void fullscreen();
	void unfullscreen();
	gboolean is_fullscreen();
	
	void set_display_mode(DisplayMode display_mode);
	void load_devices();

	bool on_timeout();
	void on_error(const Glib::ustring& message);
	void on_menu_item_open_clicked();
	void on_menu_item_close_clicked();
	void on_menu_item_quit_clicked();
	void on_menu_item_meters_clicked();
	void on_menu_item_channels_clicked();
	void on_menu_item_preferences_clicked();
	void on_menu_item_fullscreen_clicked();
	void on_menu_item_about_clicked();
	bool on_event_box_video_button_pressed(GdkEventButton* event);
	bool on_event_box_video_motion_notify_event(GdkEventMotion* event);
	bool on_event_box_video_scroll_event(GdkEventScroll* event);
	void on_button_epg_previous_clicked();
	void on_button_epg_now_clicked();
	void on_button_epg_next_clicked();
	bool on_drawing_area_video_expose(GdkEventExpose* expose);

public:
	MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade);
	~MainWindow();

	Gtk::DrawingArea& get_drawing_area();
};

#endif
