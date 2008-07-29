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
		const Channel* display_channel = profile.get_display_channel();
		const ChannelList& channels = profile.get_channels();
		guint epg_span_hours = get_application().get_int_configuration_value("epg_span_hours");

		table_epg->resize(epg_span_hours * 6 + 1, channels.size() + 1);

		guint start_time = time(NULL) + timezone; + offset;
		start_time = (start_time / 600) * 600;
		
		guint row = 1;
		for (ChannelList::const_iterator iterator = channels.begin(); iterator != channels.end(); iterator++)
		{
			const Channel& channel = *iterator;
			gboolean selected = display_channel != NULL && channel.channel_id == display_channel->channel_id;
			create_channel_row(channel, row++, selected, start_time, epg_span_hours);
		}
		get_window()->thaw_updates();
		
		show_all();
	}
}

void GtkEpgWidget::create_channel_row(const Channel& channel, guint table_row, gboolean selected, guint start_time, guint epg_span_hours)
{	
	Gtk::ToggleButton& channel_button = attach_toggle_button("<b>" + channel.name + "</b>", 0, 1, table_row, table_row + 1);
	
	channel_button.set_active(selected);
	
	channel_button.signal_clicked().connect(
		sigc::bind<guint>
		(
			sigc::mem_fun(*this, &GtkEpgWidget::on_button_channel_name_clicked),
			channel.channel_id
		)
	);
	
	const EpgEventList& events = data.get_epg_events(channel, start_time, start_time+(epg_span_hours*60*60));
	guint total_number_columns = 0;
	guint end_time = start_time + epg_span_hours*60*60;
	guint last_event_end_time = 0;
	guint number_columns = epg_span_hours * 6 + 1;
	for (EpgEventList::const_iterator i = events.begin(); i != events.end(); i++)
	{		
		const EpgEvent& event = *i;
					
		guint event_start_time = event.start_time;
		guint event_duration = event.duration;
		guint event_end_time = event_start_time + event_duration;

		if ((event_end_time > start_time && event_end_time < end_time) ||
			(event_start_time < end_time && event_start_time > start_time) ||
			(event_start_time < start_time && event_end_time > end_time)
		)
		{
			Glib::ustring time_text;
			
			guint start_column = 0;
			if (event_start_time < start_time)
			{
				start_column = 0;
			}
			else
			{
				start_column = (guint)round((event_start_time - start_time) / 600.0);
			}
			
			guint end_column = (guint)round((event_end_time - start_time) / 600.0);
			if (end_column > number_columns-1)
			{
				end_column = number_columns-1;
			}
			
			guint column_count = end_column - start_column;
			if (start_column >= total_number_columns && column_count > 0)
			{
				// If there's a gap, plug it
				if (start_column > total_number_columns)
				{
					// If it's a small gap then just extend the event
					if (event_start_time - last_event_end_time <= 2 * 60)
					{
						start_column = total_number_columns;
						column_count = end_column - start_column;
					}
					else
					{
						guint empty_columns = start_column - total_number_columns;
						attach_button(UNKNOWN_TEXT, total_number_columns + 1, start_column + 1, table_row, table_row + 1);
						total_number_columns += empty_columns;
					}
				}
				
				if (column_count > 0)
				{
					Glib::ustring time_string = get_time_string(event.start_time - timezone, "%d/%m, %H:%M");
					time_string += get_time_string(event.start_time - timezone + event.duration, " - %H:%M");
					Glib::ustring text = "<i><small>" + time_string + "</small></i>\n" + encode_xml(event.get_title());
					
					Gtk::Button& button = attach_button(text, start_column + 1, end_column + 1, table_row, table_row + 1);
					button.signal_clicked().connect(
						sigc::bind<guint>
						(
							sigc::mem_fun(*this, &GtkEpgWidget::on_button_program_clicked),
							event.epg_event_id
						)
					);
				}

				total_number_columns += column_count;
			}
			last_event_end_time = event_end_time;
		}
	}
	
	if (total_number_columns < number_columns-1)
	{		
		attach_button(UNKNOWN_TEXT, total_number_columns + 1, number_columns, table_row, table_row + 1);
	}
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

void GtkEpgWidget::on_button_program_clicked(guint epg_event_id)
{
	TRY
	/*
	Gtk::Dialog* dialog = dynamic_cast<Gtk::Dialog*>(glade->get_widget("dialog_program_details"));
	if (dialog == NULL)
	{
		throw Exception("Failed to load dialog");
	}
	dynamic_cast<Gtk::Label*>(glade->get_widget("label_program_title"))->set_text("Text & text");
	dialog->run();
	*/
	CATCH
}
