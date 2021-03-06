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

#ifndef __GTK_EPG_WIDGET__
#define __GTK_EPG_WIDGET__

#include <gtkmm.h>
#include "data.h"
#include "me-tv-ui.h"

class GtkEpgWidget: public Gtk::ScrolledWindow {
private:
	gint offset;
	gsize span_hours;
	gsize span_minutes;
	gsize span_seconds;
	Glib::RefPtr<Gtk::Builder> const builder;
	guint epg_span_hours;
	Gtk::SpinButton * spin_button_epg_page;
	Gtk::Label * label_epg_page;
	Gtk::Table * table_epg;
	Gtk::ScrolledWindow * scrolled_window_epg;
	void previous();
	void next();
	void previous_day();
	void next_day();
	bool on_channel_button_press_event(GdkEventButton * event, guint channel_id);
	void on_channel_button_toggled(Gtk::RadioButton * button, guint channel_id);
	bool on_program_button_press_event(GdkEventButton * event, EpgEvent & epg_event);
	void on_program_button_clicked(EpgEvent & epg_event);
	void on_spin_button_epg_page_changed();
	void clear();
	void update_pages();
	Gtk::RadioButton & attach_radio_button(Gtk::RadioButtonGroup & group, Glib::ustring const & text, gboolean record, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options = Gtk::FILL);
	Gtk::Button& attach_button(Glib::ustring const & text, gboolean record, gboolean ellipsize, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options = Gtk::FILL);
	Gtk::Label& attach_label(Glib::ustring const & text, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options = Gtk::FILL);
	void attach_widget(Gtk::Widget & widget, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options = Gtk::FILL);
	void create_channel_row(Gtk::RadioButtonGroup & group, Channel & const_channel,
		guint table_row, gboolean selected, time_t start_time, guint channel_number,
		gboolean show_channel_number, gboolean show_epg_time, gboolean show_epg_tooltips);
public:
	GtkEpgWidget(BaseObjectType * cobject, Glib::RefPtr<Gtk::Builder> const & builder);
	void update_table();
	void update();
	void set_offset(gint value);
	void set_epg_page(gint value);
	void increment_offset(gint value);
};

#endif
