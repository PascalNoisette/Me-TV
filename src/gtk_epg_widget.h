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

#ifndef __GTK_EPG_WIDGET__
#define __GTK_EPG_WIDGET__

#include <libgnomeuimm.h>
#include <libglademm.h>
#include "data.h"

class GtkEpgWidget : public Gtk::ScrolledWindow
{
private:
	gint offset;
	gsize span_hours;
	gsize span_minutes;
	gsize span_seconds;
	const Glib::RefPtr<Gnome::Glade::Xml> glade;
	guint epg_span_hours;
	
	Gtk::Table* table_epg;
	Gtk::ScrolledWindow* scrolled_window_epg;
	
	void previous();
	void next();
		
	void on_button_program_clicked(EpgEvent& epg_event);
	void on_button_channel_name_clicked(guint channel_id);
		
	void clear();
	void update_table();
	Gtk::ToggleButton& attach_toggle_button(const Glib::ustring& text, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options = Gtk::FILL);
	Gtk::Button& attach_button(const Glib::ustring& text, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options = Gtk::FILL);
	Gtk::Label& attach_label(const Glib::ustring& text, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options = Gtk::FILL);
	void attach_widget(Gtk::Widget& widget, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options = Gtk::FILL);
	void create_channel_row(const Channel& channel, guint row, gboolean selected, guint start_time);

public:
	GtkEpgWidget(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade);
	
	void update();
	void set_offset(guint value);
	void increment_offset(gint value);
};

#endif
