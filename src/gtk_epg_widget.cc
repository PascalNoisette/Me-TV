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

#include "gtk_epg_widget.h"
#include "application.h"
#include "scheduled_recording_dialog.h"

#define MINUTES_PER_COLUMN	1
#define COLUMNS_PER_HOUR	(60 * MINUTES_PER_COLUMN)

GtkEpgWidget::GtkEpgWidget(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::ScrolledWindow(cobject), builder(builder)
{
	offset = 0;
	epg_page = 0;

	builder->get_widget("table_epg", table_epg);
	builder->get_widget("scrolled_window_epg", scrolled_window_epg);
	
	epg_span_hours = get_application().get_int_configuration_value("epg_span_hours");
	Gtk::Button* button = NULL;
	
	builder->get_widget("button_epg_now", button);
	button->signal_clicked().connect(sigc::bind<gint>(sigc::mem_fun(*this, &GtkEpgWidget::set_offset), 0));

	builder->get_widget("button_epg_previous", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &GtkEpgWidget::previous));

	builder->get_widget("button_epg_next", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &GtkEpgWidget::next));

	builder->get_widget("label_epg_page", label_epg_page);
	builder->get_widget_derived("combo_box_epg_page", combo_box_epg_page);
	combo_box_epg_page->signal_changed().connect(sigc::mem_fun(*this, &GtkEpgWidget::on_combo_box_epg_page_changed));
}

void GtkEpgWidget::set_offset(gint value)
{
	if (value < 0)
	{
		value = 0;
	}
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

void GtkEpgWidget::on_combo_box_epg_page_changed()
{	
	TRY
	if (combo_box_epg_page->get_size() > 0)
	{
		try
		{
			epg_page = combo_box_epg_page->get_active_value();
		}
		catch(...)
		{
			g_debug("Ignoring invalid active integer value");
		}
		update_table();
	}
	CATCH
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
	Application& application = get_application();
	guint epg_page_count = combo_box_epg_page->get_size();
	guint epg_page_size = application.get_int_configuration_value("epg_page_size");

	if (epg_page_size == 0)
	{
		return;
	}
	
	const ChannelArray& channels = application.channel_manager.get_channels();
	guint channel_count = channels.size();
	guint new_epg_page_count = channel_count == 0 ? 1 : ((channel_count-1) / epg_page_size) + 1;
	
	if (epg_page == 0 && new_epg_page_count >= 1)
	{
		epg_page = 1;
	}
	
	if (new_epg_page_count != epg_page_count)
	{		
		combo_box_epg_page->set_size(new_epg_page_count);
	}
	
	label_epg_page->property_visible() = new_epg_page_count > 1;
	combo_box_epg_page->property_visible() = new_epg_page_count > 1;
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
		
		epg_span_hours = get_application().get_int_configuration_value("epg_span_hours");

		Gtk::Widget* widget = NULL;

		builder->get_widget("button_epg_previous", widget);
		widget->set_sensitive(offset > 0);
		
		builder->get_widget("button_epg_now", widget);
		widget->set_sensitive(offset > 0);
		
		ChannelManager& channel_manager = get_application().channel_manager;
		guint display_channel_index = -1;
		if (channel_manager.has_display_channel())
		{
			display_channel_index = channel_manager.get_display_channel_index();
		}
		ChannelArray& channels = channel_manager.get_channels();

		table_epg->resize(epg_span_hours * COLUMNS_PER_HOUR + 1, channels.size() + 1);

		guint start_time = time(NULL) + offset;
		start_time = (start_time / COLUMNS_PER_HOUR) * COLUMNS_PER_HOUR;
		
		guint row = 0;
		gboolean show_epg_header = get_application().get_boolean_configuration_value("show_epg_header");
		if (show_epg_header)
		{
			for (guint hour = 0; hour < epg_span_hours; hour++)
			{
				guint hour_time = start_time + (hour * 60 * 60);
				Glib::ustring hour_time_text = get_local_time_text(hour_time, "%c");
				Gtk::Button& button = attach_button(hour_time_text, hour * COLUMNS_PER_HOUR + 1, (hour+1) * COLUMNS_PER_HOUR + 1, 0, 1, Gtk::FILL | Gtk::EXPAND);
				button.set_sensitive(false);
			}
			row++;
		}
		start_time += timezone;

		guint epg_page_size = get_application().get_int_configuration_value("epg_page_size");
		guint channel_count = 0;
		guint channel_start = (epg_page-1) * epg_page_size;
		guint channel_end = channel_start + epg_page_size;
		for (ChannelArray::iterator iterator = channels.begin(); iterator != channels.end(); iterator++)
		{
			if (channel_start <= channel_count && channel_count < channel_end)
			{
				Channel& channel = *iterator;
				
				if (channel.channel_id == 0)
				{
					throw Exception(_("Failed to a create channel row because the channel ID was 0"));
				}
				
				gboolean selected = channel_count == display_channel_index;
				create_channel_row(channel, row++, selected, start_time);
			}
			
			channel_count++;
		}
		get_window()->thaw_updates();
	}
	hadjustment->set_value(hvalue);
	vadjustment->set_value(vvalue);
}

void GtkEpgWidget::create_channel_row(const Channel& const_channel, guint table_row, gboolean selected, guint start_time)
{	
	Channel channel = const_channel;
	
	Glib::ustring channel_text = Glib::ustring::compose("<i>%1.</i> <b>%2</b>", table_row + 1, encode_xml(channel.name));
	Gtk::ToggleButton& channel_button = attach_toggle_button( channel_text, 0, 1, table_row, table_row + 1);
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
	guint number_columns = epg_span_hours * COLUMNS_PER_HOUR + 1;

	if (!disable_epg)
	{		
		EpgEventList events = channel.epg_events.get_list();
		for (EpgEventList::const_iterator i = events.begin(); i != events.end(); i++)
		{
			const EpgEvent& epg_event = *i;
					
			if (
				(epg_event.start_time >= start_time && epg_event.start_time <= end_time) ||
				(epg_event.get_end_time() >= start_time && epg_event.get_end_time() <= end_time) ||
				(epg_event.start_time <= start_time && epg_event.get_end_time() >= end_time)
			)
			{
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
							empty_columns < 10 ? _("-") : _("Unknown program"),
							total_number_columns + 1, start_column + 1, table_row, table_row + 1);
						button.set_sensitive(false);
						total_number_columns += empty_columns;
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
		}
	}
	
	if (total_number_columns < number_columns-1)
	{
		guint empty_columns = (number_columns-1) - total_number_columns;
		Gtk::Button& button = attach_button(
			empty_columns < 10 ? _("-") : _("Unknown program"),
			total_number_columns + 1, number_columns, table_row, table_row + 1);
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
	get_application().set_display_channel_by_id(channel_id);
	CATCH

	TRY
	update_table();
	CATCH
}

void GtkEpgWidget::on_button_program_clicked(EpgEvent& epg_event)
{
	TRY
		
	FullscreenBugWorkaround fullscreen_bug_workaround;

	Gtk::Dialog* dialog_program_details = NULL;
	builder->get_widget("dialog_program_details", dialog_program_details);

	Gtk::TextView* text_view = NULL;
	builder->get_widget("text_view_program_title", text_view);
	text_view->get_buffer()->assign(epg_event.get_title());

	builder->get_widget("text_view_program_description", text_view);
	text_view->get_buffer()->assign(epg_event.get_description());

	builder->get_widget("text_view_program_start_time", text_view);
	text_view->get_buffer()->assign(epg_event.get_start_time_text());

	builder->get_widget("text_view_program_duration", text_view);
	text_view->get_buffer()->assign(epg_event.get_duration_text());

	gint result = dialog_program_details->run();
	dialog_program_details->hide();

	if (result == 1)
	{
		ScheduledRecordingDialog& scheduled_recording_dialog = ScheduledRecordingDialog::create(builder);
		scheduled_recording_dialog.run(MainWindow::create(builder), epg_event);
		scheduled_recording_dialog.hide();
	}
	
	CATCH
}
