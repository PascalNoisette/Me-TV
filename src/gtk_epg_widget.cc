/*
 * Copyright (C) 2010 Michael Lamothe
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
#include "scheduled_recording_dialog.h"
#include "epg_event_dialog.h"

#define MINUTES_PER_COLUMN	1
#define COLUMNS_PER_HOUR	(60 * MINUTES_PER_COLUMN)

GtkEpgWidget::GtkEpgWidget(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::ScrolledWindow(cobject), builder(builder)
{
	offset = 0;

	builder->get_widget("table_epg", table_epg);
	builder->get_widget("scrolled_window_epg", scrolled_window_epg);
	
	epg_span_hours = configuration_manager.get_int_value("epg_span_hours");
	Gtk::Button* button = NULL;
	
	builder->get_widget("button_epg_now", button);
	button->signal_clicked().connect(sigc::bind<gint>(sigc::mem_fun(*this, &GtkEpgWidget::set_offset), 0));

	builder->get_widget("button_epg_previous", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &GtkEpgWidget::previous));

	builder->get_widget("button_epg_next", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &GtkEpgWidget::next));

	builder->get_widget("button_epg_previous_day", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &GtkEpgWidget::previous_day));

	builder->get_widget("button_epg_next_day", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &GtkEpgWidget::next_day));

	builder->get_widget("label_epg_page", label_epg_page);
	builder->get_widget("spin_button_epg_page", spin_button_epg_page);
	spin_button_epg_page->signal_changed().connect(sigc::mem_fun(*this, &GtkEpgWidget::on_spin_button_epg_page_changed));
}

void GtkEpgWidget::set_offset(gint value)
{
	offset = value;
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

void GtkEpgWidget::previous_day()
{
	set_offset(offset - (24*60*60));
}

void GtkEpgWidget::next_day()
{
	set_offset(offset + (24*60*60));
}

void GtkEpgWidget::on_spin_button_epg_page_changed()
{	
	update_table();
}

void GtkEpgWidget::update()
{
	g_debug("Updating EPG");
	
	update_pages();
	update_table();
	
	g_debug("EPG update complete");
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

void GtkEpgWidget::update_pages()
{
	guint epg_page_size = configuration_manager.get_int_value("epg_page_size");
	if (epg_page_size == 0)
	{
		return;
	}

	double min = 0, max = 0;
	spin_button_epg_page->get_range(min, max);
	
	guint epg_page_count = max;
	
	const ChannelArray& channels = channel_manager.get_channels();
	guint channel_count = channels.size();
	guint new_epg_page_count = channel_count == 0 ? 1 : ((channel_count-1) / epg_page_size) + 1;
	
	if (new_epg_page_count != epg_page_count)
	{		
		spin_button_epg_page->set_range(1, new_epg_page_count);
	}
	
	label_epg_page->property_visible() = new_epg_page_count > 1;
	spin_button_epg_page->property_visible() = new_epg_page_count > 1;
}

void GtkEpgWidget::update_table()
{
	Gtk::Adjustment* hadjustment = scrolled_window_epg->get_hadjustment();
	Gtk::Adjustment* vadjustment = scrolled_window_epg->get_vadjustment();
	
	gdouble hvalue = hadjustment->get_value();
	gdouble vvalue = vadjustment->get_value();

	if (get_window())
	{
		get_window()->freeze_updates();
		
		clear();
		
		epg_span_hours = configuration_manager.get_int_value("epg_span_hours");

		Gtk::Widget* widget = NULL;

		builder->get_widget("button_epg_now", widget);
		widget->set_sensitive(offset != 0);
		
		ChannelArray& channels = channel_manager.get_channels();

		table_epg->resize(epg_span_hours * COLUMNS_PER_HOUR + 1, channels.size() + 1);

		guint start_time = time(NULL) + offset;
		start_time = (start_time / COLUMNS_PER_HOUR) * COLUMNS_PER_HOUR;
		
		guint row = 0;
		gboolean show_epg_header = configuration_manager.get_boolean_value("show_epg_header");
		if (show_epg_header)
		{
			for (guint hour = 0; hour < epg_span_hours; hour++)
			{
				guint hour_time = start_time + (hour * 60 * 60);
				Glib::ustring hour_time_text = get_local_time_text(hour_time, "%c");

				Gtk::Frame* frame = Gtk::manage(new Gtk::Frame());
				Gtk::Label* label = Gtk::manage(new Gtk::Label(hour_time_text));
				label->set_alignment(0,0.5);
				label->show();
				frame->add(*label);
				frame->set_shadow_type(Gtk::SHADOW_OUT);
				
				attach_widget(*frame,
				    hour * COLUMNS_PER_HOUR + 1,
				    (hour+1) * COLUMNS_PER_HOUR + 1,
				    0, 1, Gtk::FILL | Gtk::EXPAND);
			}
			row++;
		}
		start_time += timezone;

		gint epg_page = spin_button_epg_page->is_visible() ? spin_button_epg_page->get_value_as_int() : 1;
		guint epg_page_size = configuration_manager.get_int_value("epg_page_size");
		gboolean show_channel_number = configuration_manager.get_boolean_value("show_channel_number");
		gboolean show_epg_time = configuration_manager.get_boolean_value("show_epg_time");
		gboolean show_epg_tooltips = configuration_manager.get_boolean_value("show_epg_tooltips");
		guint channel_start = (epg_page-1) * epg_page_size;
		guint channel_end = channel_start + epg_page_size;

		if (channel_end >= channels.size())
		{
			channel_end = channels.size();
		}

		Gtk::RadioButtonGroup group;
		for (guint channel_index = channel_start; channel_index < channel_end; channel_index++)
		{
			Channel& channel = channels[channel_index];
			gboolean selected = stream_manager.has_display_stream() && (stream_manager.get_display_channel() == channel);
			create_channel_row(group, channel, row++, selected, start_time, channel_index + 1,
				show_channel_number, show_epg_time, show_epg_tooltips);
		}
		get_window()->thaw_updates();
	}
	hadjustment->set_value(hvalue);
	vadjustment->set_value(vvalue);
}

void GtkEpgWidget::create_channel_row(Gtk::RadioButtonGroup& group, Channel& channel,
	guint table_row, gboolean selected, time_t start_time, guint channel_number,
	gboolean show_channel_number, gboolean show_epg_time, gboolean show_epg_tooltips)
{
	Glib::ustring channel_text = Glib::ustring::compose("<b>%1</b>", encode_xml(channel.name));
	if (show_channel_number)
	{
		channel_text = Glib::ustring::compose("<i>%1.</i> ", channel_number) + channel_text;
	}
	
	gboolean record_channel = stream_manager.is_recording(channel);
	
	Gtk::RadioButton& channel_button = attach_radio_button(group, channel_text, record_channel, 0, 1, table_row, table_row + 1);

	if (selected)
	{
		Gtk::HBox* hbox = dynamic_cast<Gtk::HBox*>(channel_button.get_child());
		Gtk::Image* image = Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON));
		image->set_alignment(1, 0.5);
		hbox->pack_start(*image, false, false);
		image->show();
	}
	
	channel_button.set_active(selected);
	channel_button.signal_toggled().connect(
		sigc::bind<Gtk::RadioButton*, guint>
		(
			sigc::mem_fun(*this, &GtkEpgWidget::on_channel_button_toggled),
			&channel_button, channel.channel_id
		),
	    false
	);
	channel_button.signal_button_press_event().connect(
		sigc::bind<guint>
		(
			sigc::mem_fun(*this, &GtkEpgWidget::on_channel_button_press_event),
			channel.channel_id
		),
	    false
	);
	
	guint total_number_columns = 0;
	time_t end_time = start_time + epg_span_hours*60*60;
	guint last_event_end_time = 0;
	guint number_columns = epg_span_hours * COLUMNS_PER_HOUR + 1;

	if (!disable_epg)
	{
		EpgEventList events = channel.epg_events.get_list(start_time, end_time);
		for (EpgEventList::iterator i = events.begin(); i != events.end(); i++)
		{
			EpgEvent& epg_event = *i;

			guint event_end_time = epg_event.start_time + epg_event.duration;		
			guint start_column = 0;
			if (epg_event.start_time < start_time)
			{
				start_column = 0;
			}
			else
			{
				start_column = (guint)round((epg_event.start_time - start_time) / COLUMNS_PER_HOUR);
			}
		
			guint end_column = (guint)round((event_end_time - start_time) / COLUMNS_PER_HOUR);
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
					guint empty_columns = start_column - total_number_columns;
					Gtk::Button& button = attach_button(
						empty_columns < 10 ? _("-") : _("Unknown program"), false,
						total_number_columns + 1, start_column + 1, table_row, table_row + 1);
					button.set_sensitive(false);
					total_number_columns += empty_columns;
				}
			
				if (column_count > 0)
				{
					guint converted_start_time = convert_to_utc_time(epg_event.start_time);

					Glib::ustring text;
					if (show_epg_time)
					{
						text = get_local_time_text(converted_start_time, "<b>%H:%M");
						text += get_local_time_text(converted_start_time + epg_event.duration, " - %H:%M</b>\n");
					}
					text += encode_xml(epg_event.get_title());

					gboolean record = scheduled_recording_manager.is_recording(epg_event);
					
					Gtk::Button& button = attach_button(text, record, start_column + 1, end_column + 1, table_row, table_row + 1);
					button.signal_clicked().connect(
						sigc::bind<EpgEvent>
						(
							sigc::mem_fun(*this, &GtkEpgWidget::on_program_button_clicked),
							epg_event
						)
					);
					button.signal_button_press_event().connect(
						sigc::bind<EpgEvent>
						(
							sigc::mem_fun(*this, &GtkEpgWidget::on_program_button_press_event),
							epg_event
						),
					    false
					);

					if (show_epg_tooltips)
					{
						Glib::ustring tooltip_text = epg_event.get_title();
						tooltip_text += get_local_time_text(converted_start_time, "\n%A, %B %d (%H:%M");
						tooltip_text += get_local_time_text(converted_start_time + epg_event.duration, " - %H:%M)");
						Glib::ustring subtitle = trim_string(epg_event.get_subtitle());
						Glib::ustring description = trim_string(epg_event.get_description());
						if (!subtitle.empty())
						{
							tooltip_text += "\n" + subtitle;
							if (!description.empty())
							{
								tooltip_text += " - ";
							}
						}
						else if (!description.empty())
						{
							tooltip_text += "\n";
						}
						tooltip_text += description;
						button.set_tooltip_text(tooltip_text);
					}
				}

				total_number_columns += column_count;
			}
			last_event_end_time = event_end_time;
		}
	}
	
	if (total_number_columns < number_columns-1)
	{
		guint empty_columns = (number_columns-1) - total_number_columns;
		Gtk::Button& button = attach_button(
			empty_columns < 10 ? _("-") : _("Unknown program"), false,
			total_number_columns + 1, number_columns, table_row, table_row + 1);
		button.set_sensitive(false);
	}
}

Gtk::RadioButton& GtkEpgWidget::attach_radio_button(Gtk::RadioButtonGroup& group, const Glib::ustring& text, gboolean record, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options)
{
	Gtk::RadioButton* button = Gtk::manage(new Gtk::RadioButton(group));
	button->property_draw_indicator() = false;
	button->set_active(false);
	button->set_alignment(0, 0.5);

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());

	Gtk::Label* label = Gtk::manage(new Gtk::Label(text));
	label->set_use_markup(true);
	label->set_alignment(0, 0.5);
	label->set_padding(3, 0);
	hbox->pack_start(*label, true, true);

	if (record == true)
	{
		Gtk::Image* image = Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_RECORD, Gtk::ICON_SIZE_BUTTON));
		image->set_alignment(1, 0.5);
		hbox->pack_end(*image, false, false);
	}
	
	button->add(*hbox);
	button->show_all();

	attach_widget(*button, left_attach, right_attach, top_attach, bottom_attach, attach_options);

	return *button;
}

Gtk::Button& GtkEpgWidget::attach_button(const Glib::ustring& text, gboolean record, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options)
{
	Gtk::Button* button = new Gtk::Button();
	button->set_alignment(0, 0.5);

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());

	Gtk::Label* label = Gtk::manage(new Gtk::Label(text));
	label->set_use_markup(true);
	label->set_alignment(0, 0.5);
	label->set_ellipsize(Pango::ELLIPSIZE_END);
	hbox->pack_start(*label, true, true);

	if (record == true)
	{
		Gtk::Image* image = Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_RECORD, Gtk::ICON_SIZE_BUTTON));
		hbox->pack_end(*image, false, false);
	}
		
	button->add(*hbox);	
	button->show_all();

	attach_widget(*button, left_attach, right_attach, top_attach, bottom_attach, attach_options);
	
	return *button;
}

Gtk::Label& GtkEpgWidget::attach_label(const Glib::ustring& text, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options)
{
	Gtk::Label* label = new Gtk::Label(text.c_str());
	label->set_justify(Gtk::JUSTIFY_LEFT);
	label->set_use_markup(true);
	attach_widget(*label, left_attach, right_attach, top_attach, bottom_attach, attach_options);
	return *label;
}

void GtkEpgWidget::attach_widget(Gtk::Widget& widget, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options)
{
	table_epg->attach(widget, left_attach, right_attach, top_attach, bottom_attach, attach_options, Gtk::FILL, 0, 0);
	widget.show();
}

bool GtkEpgWidget::on_channel_button_press_event(GdkEventButton* event, guint channel_id)
{		
	if (event->type == GDK_BUTTON_PRESS && event->button == 3)
	{
		device_manager.check_frontend();
		
		Channel& channel = channel_manager.get_channel_by_id(channel_id);

		if (stream_manager.is_recording(channel))
		{
			get_application().stop_recording(channel);
		}
		else
		{
			get_application().start_recording(channel);
		}

		update_table();
	}

	return false;
}

void GtkEpgWidget::on_channel_button_toggled(Gtk::RadioButton* button, guint channel_id)
{
	if (button->get_active())
	{
		signal_start_display(channel_id);
	}
}

bool GtkEpgWidget::on_program_button_press_event(GdkEventButton* event, EpgEvent& epg_event)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 3)
	{
		if (scheduled_recording_manager.is_recording(epg_event))
		{
			scheduled_recording_manager.remove_scheduled_recording(epg_event);
		}
		else
		{
			scheduled_recording_manager.set_scheduled_recording(epg_event);
		}

		signal_update();
	}

	return false;
}

void GtkEpgWidget::on_program_button_clicked(EpgEvent& epg_event)
{
	EpgEventDialog::create(builder).show_epg_event(epg_event);
}
