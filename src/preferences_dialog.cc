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

#include "preferences_dialog.h"
#include "application.h"

PreferencesDialog* PreferencesDialog::create(Glib::RefPtr<Gnome::Glade::Xml> glade)
{
	PreferencesDialog* preferences_dialog = NULL;
	glade->get_widget_derived("dialog_preferences", preferences_dialog);
	return preferences_dialog;
}

PreferencesDialog::PreferencesDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml)
	: Gtk::Dialog(cobject), glade(glade_xml)
{
}

void PreferencesDialog::run()
{
	Application& application = get_application();
	
	Gtk::FileChooserButton* file_chooser_button_recording_directory = dynamic_cast<Gtk::FileChooserButton*>(glade->get_widget("file_chooser_button_recording_directory"));
	Gtk::SpinButton* spin_button_record_extra_before = dynamic_cast<Gtk::SpinButton*>(glade->get_widget("spin_button_record_extra_before"));
	Gtk::SpinButton* spin_button_record_extra_after = dynamic_cast<Gtk::SpinButton*>(glade->get_widget("spin_button_record_extra_after"));
	Gtk::SpinButton* spin_button_epg_span_hours = dynamic_cast<Gtk::SpinButton*>(glade->get_widget("spin_button_epg_span_hours"));
	Gtk::Entry* entry_broadcast_address = dynamic_cast<Gtk::Entry*>(glade->get_widget("entry_broadcast_address"));
	Gtk::SpinButton* spin_button_broadcast_port = dynamic_cast<Gtk::SpinButton*>(glade->get_widget("spin_button_broadcast_port"));
	Gtk::Entry* entry_preferred_language = dynamic_cast<Gtk::Entry*>(glade->get_widget("entry_preferred_language"));
	Gtk::Entry* entry_xine_video_driver = dynamic_cast<Gtk::Entry*>(glade->get_widget("entry_xine_video_driver"));
	Gtk::Entry* entry_xine_audio_driver = dynamic_cast<Gtk::Entry*>(glade->get_widget("entry_xine_audio_driver"));
	Gtk::Entry* entry_text_encoding = dynamic_cast<Gtk::Entry*>(glade->get_widget("entry_text_encoding"));
	Gtk::CheckButton* check_button_keep_above = dynamic_cast<Gtk::CheckButton*>(glade->get_widget("check_button_keep_above"));
	Gtk::CheckButton* check_button_show_epg_header = dynamic_cast<Gtk::CheckButton*>(glade->get_widget("check_button_show_epg_header"));
	Gtk::CheckButton* check_button_show_epg_time = dynamic_cast<Gtk::CheckButton*>(glade->get_widget("check_button_show_epg_time"));
	Gtk::CheckButton* check_button_show_epg_tooltips = dynamic_cast<Gtk::CheckButton*>(glade->get_widget("check_button_show_epg_tooltips"));
	Gtk::CheckButton* check_button_24_hour_workaround = dynamic_cast<Gtk::CheckButton*>(glade->get_widget("check_button_24_hour_workaround"));
	
	file_chooser_button_recording_directory->set_filename(application.get_string_configuration_value("recording_directory"));
	spin_button_record_extra_before->set_value(application.get_int_configuration_value("record_extra_before"));
	spin_button_record_extra_after->set_value(application.get_int_configuration_value("record_extra_after"));
	spin_button_epg_span_hours->set_value(application.get_int_configuration_value("epg_span_hours"));
	entry_broadcast_address->set_text(application.get_string_configuration_value("broadcast_address"));
	spin_button_broadcast_port->set_value(application.get_int_configuration_value("broadcast_port"));
	entry_preferred_language->set_text(application.get_string_configuration_value("preferred_language"));
	entry_xine_video_driver->set_text(application.get_string_configuration_value("xine.video_driver"));
	entry_xine_audio_driver->set_text(application.get_string_configuration_value("xine.audio_driver"));
	entry_text_encoding->set_text(application.get_string_configuration_value("text_encoding"));
	check_button_keep_above->set_active(application.get_boolean_configuration_value("keep_above"));
	check_button_show_epg_header->set_active(application.get_boolean_configuration_value("show_epg_header"));
	check_button_show_epg_time->set_active(application.get_boolean_configuration_value("show_epg_time"));
	check_button_show_epg_tooltips->set_active(application.get_boolean_configuration_value("show_epg_tooltips"));
	check_button_24_hour_workaround->set_active(application.get_boolean_configuration_value("use_24_hour_workaround"));
	
	if (Dialog::run() == Gtk::RESPONSE_OK)
	{
		application.set_string_configuration_value("recording_directory", file_chooser_button_recording_directory->get_filename());
		application.set_int_configuration_value("record_extra_before", spin_button_record_extra_before->get_value());
		application.set_int_configuration_value("record_extra_after", spin_button_record_extra_after->get_value());
		application.set_int_configuration_value("epg_span_hours", spin_button_epg_span_hours->get_value());
		application.set_string_configuration_value("broadcast_address", entry_broadcast_address->get_text());
		application.set_int_configuration_value("broadcast_port", spin_button_broadcast_port->get_value());
		application.set_string_configuration_value("preferred_language", entry_preferred_language->get_text());
		application.set_string_configuration_value("xine.video_driver", entry_xine_video_driver->get_text());
		application.set_string_configuration_value("xine.audio_driver", entry_xine_audio_driver->get_text());
		application.set_string_configuration_value("text_encoding", entry_text_encoding->get_text());
		application.set_boolean_configuration_value("keep_above", check_button_keep_above->get_active());
		application.set_boolean_configuration_value("show_epg_header", check_button_show_epg_header->get_active());
		application.set_boolean_configuration_value("show_epg_time", check_button_show_epg_time->get_active());
		application.set_boolean_configuration_value("show_epg_tooltips", check_button_show_epg_tooltips->get_active());
		application.set_boolean_configuration_value("use_24_hour_workaround", check_button_24_hour_workaround->get_active());

		get_application().update();
	}
}
