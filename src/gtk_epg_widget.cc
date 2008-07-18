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
		
		ChannelManager& channel_manager = get_application().get_channel_manager();
		const Channel& display_channel = channel_manager.get_display_channel();
		const ChannelList& channels = channel_manager.get_channels();
		Glib::RefPtr<Gnome::Conf::Client> client = Gnome::Conf::Client::get_default_client();
		guint epg_span_hours = client->get_int(GCONF_PATH"/epg_span_hours");

		table_epg->resize(epg_span_hours * 6 + 1, channels.size() + 1);

		Gtk::Button& button_previous = attach_button("<b>&lt;</b>", 1, 2, 0, 1);
		time_t t = time(NULL);
		char buffer[1000];
		struct tm tp;
		for (gint hour = 0; hour < epg_span_hours; hour++)
		{
			t += 60*60;
			localtime_r(&t, &tp);
			strftime(buffer, 1000, "%x %T", &tp);
			Glib::ustring text = Glib::ustring::compose("<b>%1</b>", Glib::ustring(buffer));
			Gtk::Button& button_previous = attach_button(text, hour*6+2, (hour+1)*6+1, 0, 1);
		}
		Gtk::Button& button_next = attach_button("<b>&gt;</b>", (epg_span_hours*6)+1, (epg_span_hours*6)+2, 0, 1);	
		
		guint row = 1;
		ChannelList::const_iterator iterator = channels.begin();
		while (iterator != channels.end())
		{
			const Channel& channel = *iterator;
			create_channel_row(channel, row++, channel.name == display_channel.name, epg_span_hours);
			iterator++;
		}
		get_window()->thaw_updates();
		
		show_all();
	}
}

void GtkEpgWidget::create_channel_row(const Channel& channel, guint row, gboolean selected, guint epg_span_hours)
{
	Gtk::Button& channel_button = attach_button("<b>" + channel.name + "</b>", 0, 1, row + 1, row + 2);
	
	channel_button.signal_clicked().connect(
		sigc::bind<Glib::ustring>
		(
			sigc::mem_fun(*this, &GtkEpgWidget::on_button_channel_name_clicked),
			channel.name
		)
	);

	Gtk::Button& unknown_button = attach_button("Unknown", 1, epg_span_hours*6+2, row + 1, row + 2);
}

Gtk::Button& GtkEpgWidget::attach_button(const Glib::ustring& text,	guint left_attach, guint right_attach, guint top_attach, guint bottom_attach)
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

void GtkEpgWidget::on_button_channel_name_clicked(const Glib::ustring& channel_name)
{
	TRY
	get_application().get_channel_manager().set_display_channel(channel_name);
	CATCH
}
