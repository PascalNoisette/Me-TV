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
#include "scheduled_recording_dialog.h"

GtkEpgWidget::GtkEpgWidget(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml) :
	Gtk::ScrolledWindow(cobject), glade(glade_xml)
{
	offset = 0;

	table_epg				= dynamic_cast<Gtk::Table*>(glade->get_widget("table_epg"));
	scrolled_window_epg		= dynamic_cast<Gtk::ScrolledWindow*>(glade->get_widget("scrolled_window_epg"));
	
	epg_span_hours = get_application().get_int_configuration_value("epg_span_hours");
	glade->connect_clicked("button_epg_now",
		sigc::bind<gint>(sigc::mem_fun(*this, &GtkEpgWidget::set_offset), 0));
	glade->connect_clicked("button_epg_previous", sigc::mem_fun(*this, &GtkEpgWidget::previous));
	glade->connect_clicked("button_epg_next", sigc::mem_fun(*this, &GtkEpgWidget::next));
}

void GtkEpgWidget::set_offset(guint value)
{	
	offset = value;
	
	if (offset < 0)
	{
		offset = 0;
	}

	update();
}

void GtkEpgWidget::previous()
{
	set_offset(offset - (epg_span_hours*60*60));
}

void GtkEpgWidget::next()
{
	set_offset(offset + (epg_span_hours*60*60));
}

void GtkEpgWidget::update()
{
	g_debug("Updating EPG");
	
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
		delete first;
		iterator++;
	}
}

void GtkEpgWidget::update_table()
{
	if (get_window())
	{
		get_window()->freeze_updates();
		
		clear();
		
		epg_span_hours = get_application().get_int_configuration_value("epg_span_hours");
		
		glade->get_widget("button_epg_previous")->set_sensitive(offset > 0);
		glade->get_widget("button_epg_now")->set_sensitive(offset > 0);
			
		Profile& profile = get_application().get_profile_manager().get_current_profile();
		const Channel* display_channel = profile.get_display_channel();
		const ChannelList& channels = profile.get_channels();

		table_epg->resize(epg_span_hours * 6 + 1, channels.size() + 1);

		guint start_time = time(NULL) + offset;
		start_time = (start_time / 600) * 600;
		
		guint row = 0;
		gboolean show_epg_header = get_application().get_boolean_configuration_value("show_epg_header");
		if (show_epg_header)
		{
			for (guint hour = 0; hour < epg_span_hours; hour++)
			{
				guint hour_time = start_time + (hour * 60 * 60);
				Glib::ustring hour_time_text = get_local_time_text(hour_time, "%c");
				Gtk::Button& button = attach_button(hour_time_text, hour*6 + 1, (hour+1)*6 + 1, 0, 1, Gtk::FILL | Gtk::EXPAND);
				button.set_sensitive(false);
			}
			row++;
		}
		start_time += timezone;
		for (ChannelList::const_iterator iterator = channels.begin(); iterator != channels.end(); iterator++)
		{
			const Channel& channel = *iterator;
			gboolean selected = display_channel != NULL && channel.channel_id == display_channel->channel_id;
			create_channel_row(channel, row++, selected, start_time);
		}
		get_window()->thaw_updates();
	}
}

void GtkEpgWidget::create_channel_row(const Channel& channel, guint table_row, gboolean selected, guint start_time)
{	
	Gtk::ToggleButton& channel_button = attach_toggle_button("<b>" + channel.name + "</b>", 0, 1, table_row, table_row + 1);
	gboolean show_epg_time = get_application().get_boolean_configuration_value("show_epg_time");
	gboolean show_epg_tooltips = get_application().get_boolean_configuration_value("show_epg_tooltips");
	
	channel_button.set_active(selected);
	channel_button.signal_clicked().connect(
		sigc::bind<guint>
		(
			sigc::mem_fun(*this, &GtkEpgWidget::on_button_channel_name_clicked),
			channel.channel_id
		)
	);
	
	guint total_number_columns = 0;
	guint end_time = start_time + epg_span_hours*60*60;
	guint last_event_end_time = 0;
	guint number_columns = epg_span_hours * 6 + 1;
	
	const EpgEventList& events = data.get_epg_events(channel, start_time, end_time);
	for (EpgEventList::const_iterator i = events.begin(); i != events.end(); i++)
	{
		const EpgEvent& epg_event = *i;
			
		guint event_end_time = epg_event.start_time + epg_event.duration;		
		guint start_column = 0;
		if (epg_event.start_time < start_time)
		{
			start_column = 0;
		}
		else
		{
			start_column = (guint)round((epg_event.start_time - start_time) / 600.0);
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
				if (epg_event.start_time - last_event_end_time <= 2 * 60)
				{
					start_column = total_number_columns;
					column_count = end_column - start_column;
				}
				else
				{
					guint empty_columns = start_column - total_number_columns;
					Gtk::Button& button = attach_button(UNKNOWN_TEXT, total_number_columns + 1, start_column + 1, table_row, table_row + 1);
					button.set_sensitive(false);
					total_number_columns += empty_columns;
				}
			}
			
			if (column_count > 0)
			{
				guint converted_start_time = convert_to_utc_time (epg_event.start_time);

				Glib::ustring text;
				if (show_epg_time)
				{
					text = get_local_time_text(converted_start_time, "<b>%H:%M");
					text += get_local_time_text(converted_start_time + epg_event.duration, " - %H:%M</b>\n");
				}
				text += encode_xml(epg_event.get_title());
				
				Gtk::Button& button = attach_button(text, start_column + 1, end_column + 1, table_row, table_row + 1);
				button.signal_clicked().connect(
					sigc::bind<EpgEvent>
					(
						sigc::mem_fun(*this, &GtkEpgWidget::on_button_program_clicked),
						epg_event
					)
				);

				if (show_epg_tooltips)
				{
					Glib::ustring tooltip_text = get_local_time_text(converted_start_time, "%A, %B %d\n%H:%M");
					tooltip_text += get_local_time_text(converted_start_time + epg_event.duration, " - %H:%M");
					button.set_tooltip_text(tooltip_text);
				}
			}

			total_number_columns += column_count;
		}
		last_event_end_time = event_end_time;
	}
	
	if (total_number_columns < number_columns-1)
	{		
		Gtk::Button& button = attach_button(UNKNOWN_TEXT, total_number_columns + 1, number_columns, table_row, table_row + 1);
		button.set_sensitive(false);
	}
}

Gtk::ToggleButton& GtkEpgWidget::attach_toggle_button(const Glib::ustring& text, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options)
{
	Gtk::ToggleButton* button = new Gtk::ToggleButton(text);
	attach_widget(*button, left_attach, right_attach, top_attach, bottom_attach, attach_options);
	button->set_alignment(0, 0.5);
	Gtk::Label* label = dynamic_cast<Gtk::Label*>(button->get_child());
	label->set_use_markup(true);
	return *button;
}

Gtk::Button& GtkEpgWidget::attach_button(const Glib::ustring& text, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options)
{
	Gtk::Button* button = new Gtk::Button(text);
	attach_widget(*button, left_attach, right_attach, top_attach, bottom_attach, attach_options);
	button->set_alignment(0, 0.5);
	Gtk::Label* label = dynamic_cast<Gtk::Label*>(button->get_child());
	label->set_use_markup(true);
	return *button;
}

Gtk::Label& GtkEpgWidget::attach_label(const Glib::ustring& text, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options)
{
	Gtk::Label* label = new Gtk::Label(text.c_str());
	attach_widget(*label, left_attach, right_attach, top_attach, bottom_attach, attach_options);
	label->set_justify(Gtk::JUSTIFY_LEFT);
	label->set_use_markup(true);
	return *label;
}

void GtkEpgWidget::attach_widget(Gtk::Widget& widget, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options)
{
	table_epg->attach(widget, left_attach, right_attach, top_attach, bottom_attach, attach_options, Gtk::FILL, 0, 0);
	widget.show();
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

void GtkEpgWidget::on_button_program_clicked(EpgEvent& epg_event)
{
	TRY
	Gtk::Dialog* dialog_program_details = dynamic_cast<Gtk::Dialog*>(glade->get_widget("dialog_program_details"));
	
	(dynamic_cast<Gtk::Label*>(glade->get_widget("label_program_title")))->set_label(epg_event.get_title());
	(dynamic_cast<Gtk::Label*>(glade->get_widget("label_program_description")))->set_label(epg_event.get_description());
	(dynamic_cast<Gtk::Label*>(glade->get_widget("label_program_start_time")))->set_label(epg_event.get_start_time_text());
	(dynamic_cast<Gtk::Label*>(glade->get_widget("label_program_duration")))->set_label(epg_event.get_duration_text());
	gint result = dialog_program_details->run();
	dialog_program_details->hide();

	if (result == 1)
	{
		ScheduledRecordingDialog* scheduled_recording_dialog = ScheduledRecordingDialog::create(glade);
		scheduled_recording_dialog->run(MainWindow::create(glade), epg_event);
		scheduled_recording_dialog->hide();
	}
	
	CATCH
}
