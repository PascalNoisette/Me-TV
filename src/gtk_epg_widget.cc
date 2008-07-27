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

#include "gtk_epg_widget.h"
#include "application.h"
#include "data.h"

GtkEpgWidget::GtkEpgWidget(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) :
	Gtk::ScrolledWindow(cobject), glade(glade)
{
	offset = 0;

	table_epg			= dynamic_cast<Gtk::Table*>(glade->get_widget("table_epg"));
	scrolled_window_epg	= dynamic_cast<Gtk::ScrolledWindow*>(glade->get_widget("scrolled_window_epg"));
		
	last_number_rows = 0;
	last_number_columns = 0;
}

void GtkEpgWidget::set_offset(guint value)
{
	if (value < 0)
	{
		value = 0;
	}
	
	offset = value;
	
	update();
}

void GtkEpgWidget::increment_offset(gint value)
{
	set_offset(offset + value);
}

void GtkEpgWidget::update()
{
	Gtk::Adjustment* hadjustment = scrolled_window_epg->get_hadjustment();
	Gtk::Adjustment* vadjustment = scrolled_window_epg->get_vadjustment();
	
	gdouble hvalue = hadjustment->get_value();
	gdouble vvalue = vadjustment->get_value();

	update_table();
	
	hadjustment->set_value(hvalue);
	vadjustment->set_value(vvalue);
}

void GtkEpgWidget::clear()
{
	std::list<Gtk::Widget*> children = table_epg->get_children();
	std::list<Gtk::Widget*>::iterator iterator = children.begin();
	while (iterator != children.end())
	{
		Gtk::Widget* first = *iterator;
		table_epg->remove(*first);
		iterator++;
	}
}

void GtkEpgWidget::update_table()
{
	if (get_window())
	{
		get_window()->freeze_updates();
		
		clear();
		
		Profile& profile = get_application().get_profile_manager().get_current_profile();
		const Channel& display_channel = profile.get_display_channel();
		const ChannelList& channels = profile.get_channels();
		guint epg_span_hours = get_application().get_int_configuration_value("epg_span_hours");

		table_epg->resize(epg_span_hours * 6 + 1, channels.size() + 1);

		Gtk::Button& button_previous = attach_button("<b>&lt;</b>", 1, 2, 0, 1);
		guint start_time = time(NULL) + offset;
		start_time = (start_time / 600) * 600;
		char buffer[1000];
		struct tm tp;
		for (gint hour = 0; hour < epg_span_hours; hour++)
		{
			time_t t = start_time + hour*60*60;
			localtime_r(&t, &tp);
			strftime(buffer, 1000, "%x %T", &tp);
			Glib::ustring text = Glib::ustring::compose("<b>%1</b>", Glib::ustring(buffer));
			Gtk::Button& button_previous = attach_button(text, hour*6+2, (hour+1)*6+1, 0, 1);
		}
		Gtk::Button& button_next = attach_button("<b>&gt;</b>", (epg_span_hours*6)+1, (epg_span_hours*6)+2, 0, 1);	
		
		guint row = 1;
		for (ChannelList::const_iterator iterator = channels.begin(); iterator != channels.end(); iterator++)
		{
			const Channel& channel = *iterator;
			gboolean selected = channel.name == display_channel.name;
			create_channel_row(channel, row++, selected, start_time, epg_span_hours);
		}
		get_window()->thaw_updates();
		
		show_all();
	}
}

void GtkEpgWidget::create_channel_row(const Channel& channel, guint table_row, gboolean selected, guint start_time, guint epg_span_hours)
{	
	Gtk::ToggleButton& channel_button = attach_toggle_button("<b>" + channel.name + "</b>", 0, 1, table_row + 1, table_row + 2);
	
	channel_button.set_active(selected);
	
	channel_button.signal_clicked().connect(
		sigc::bind<guint>
		(
			sigc::mem_fun(*this, &GtkEpgWidget::on_button_channel_name_clicked),
			channel.channel_id
		)
	);
	
	/*
	EpgEventList events = data.get_epg_events(channel.frontend_parameters.frequency, channel.service_id, start_time, start_time+(epg_span_hours*60*60));
	guint start_cell = 1;
	for (EpgEventList::iterator i = events.begin(); i != events.end(); i++)
	{
		EpgEvent epg_event = *i;
		guint end_cell = start_cell+1;
		g_debug("EVENT: %s", epg_event.title.c_str());
		attach_button(epg_event.title, start_cell, end_cell, table_row + 1, table_row + 2);
		start_cell = end_cell;
	}
	*/
}

Gtk::ToggleButton& GtkEpgWidget::attach_toggle_button(const Glib::ustring& text, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach)
{
	Gtk::ToggleButton* button = new Gtk::ToggleButton(text);
	attach_widget(*button, left_attach, right_attach, top_attach, bottom_attach);
	button->set_alignment(0, 0.5);
	Gtk::Label* label = dynamic_cast<Gtk::Label*>(button->get_child());
	label->set_use_markup(true);
	return *button;
}

Gtk::Button& GtkEpgWidget::attach_button(const Glib::ustring& text, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach)
{
	Gtk::Button* button = new Gtk::Button(text);
	attach_widget(*button, left_attach, right_attach, top_attach, bottom_attach);
	button->set_alignment(0, 0.5);
	Gtk::Label* label = dynamic_cast<Gtk::Label*>(button->get_child());
	label->set_use_markup(true);
	return *button;
}

Gtk::Label& GtkEpgWidget::attach_label(const Glib::ustring& text, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach)
{
	Gtk::Label* label = new Gtk::Label(text.c_str());
	attach_widget(*label, left_attach, right_attach, top_attach, bottom_attach);
	label->set_justify(Gtk::JUSTIFY_LEFT);
	label->set_use_markup(true);
	return *label;
}

void GtkEpgWidget::attach_widget(Gtk::Widget& widget, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach)
{
	table_epg->attach(widget, left_attach, right_attach, top_attach, bottom_attach, Gtk::FILL, Gtk::FILL, 0, 0);
}

void GtkEpgWidget::on_button_channel_name_clicked(guint channel_id)
{
	TRY
	get_application().get_profile_manager().get_current_profile().set_display_channel(channel_id);
	CATCH

	TRY
	update_table();
	CATCH
}
