/*
 * Copyright (C) 2011 Michael Lamothe
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
#include "me-tv-ui.h"

PreferencesDialog& PreferencesDialog::create(Glib::RefPtr<Gtk::Builder> builder)
{
	PreferencesDialog* preferences_dialog = NULL;
	builder->get_widget_derived("dialog_preferences", preferences_dialog);
	return *preferences_dialog;
}

PreferencesDialog::PreferencesDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
	: Gtk::Dialog(cobject), builder(builder)
{
}

void PreferencesDialog::run()
{
	Gtk::FileChooserButton* file_chooser_button_recording_directory = NULL;
	Gtk::SpinButton* spin_button_record_extra_before = NULL;
	Gtk::SpinButton* spin_button_record_extra_after = NULL;
	Gtk::SpinButton* spin_button_epg_span_hours = NULL;
	Gtk::SpinButton* spin_button_epg_page_size = NULL;
	Gtk::SpinButton* spin_button_listen_port_webinterface = NULL;
	ComboBoxEntryText* combo_box_entry_video_driver = NULL;
	ComboBoxEntryText* combo_box_entry_audio_driver = NULL;
	ComboBoxText* combo_box_deinterlace_type = NULL;
	ComboBoxEntryText* combo_box_entry_text_encoding = NULL;
	Gtk::CheckButton* check_button_keep_above = NULL;
	Gtk::CheckButton* check_button_show_epg_header = NULL;
	Gtk::CheckButton* check_button_show_epg_time = NULL;
	Gtk::CheckButton* check_button_show_epg_tooltips = NULL;
	Gtk::CheckButton* check_button_display_status_icon = NULL;
	Gtk::CheckButton* check_button_show_channel_number = NULL;
	Gtk::CheckButton* check_button_remove_colon = NULL;
	Gtk::CheckButton* check_button_enable_webinterface = NULL;

	builder->get_widget("file_chooser_button_recording_directory", file_chooser_button_recording_directory);
	builder->get_widget("spin_button_record_extra_before", spin_button_record_extra_before);
	builder->get_widget("spin_button_record_extra_after", spin_button_record_extra_after);
	builder->get_widget("spin_button_epg_span_hours", spin_button_epg_span_hours);
	builder->get_widget("spin_button_epg_page_size", spin_button_epg_page_size);
	builder->get_widget("spin_button_listen_port_webinterface", spin_button_listen_port_webinterface);
	builder->get_widget_derived("combo_box_entry_video_driver", combo_box_entry_video_driver);
	builder->get_widget_derived("combo_box_entry_audio_driver", combo_box_entry_audio_driver);
	builder->get_widget_derived("combo_box_deinterlace_type", combo_box_deinterlace_type);
	builder->get_widget_derived("combo_box_entry_text_encoding", combo_box_entry_text_encoding);
	builder->get_widget("check_button_keep_above", check_button_keep_above);
	builder->get_widget("check_button_show_epg_header", check_button_show_epg_header);
	builder->get_widget("check_button_show_epg_time", check_button_show_epg_time);
	builder->get_widget("check_button_show_epg_tooltips", check_button_show_epg_tooltips);
	builder->get_widget("check_button_display_status_icon", check_button_display_status_icon);
	builder->get_widget("check_button_show_channel_number", check_button_show_channel_number);
	builder->get_widget("check_button_remove_colon", check_button_remove_colon);
	builder->get_widget("check_button_enable_webinterface", check_button_enable_webinterface);
	
	combo_box_entry_video_driver->clear_items();
	combo_box_entry_video_driver->append_text("aadxr3");
	combo_box_entry_video_driver->append_text("DirectFB");
	combo_box_entry_video_driver->append_text("dxr3");
	combo_box_entry_video_driver->append_text("fb");
	combo_box_entry_video_driver->append_text("none");
	combo_box_entry_video_driver->append_text("opengl");
	combo_box_entry_video_driver->append_text("sdl");
	combo_box_entry_video_driver->append_text("SyncFB");
	combo_box_entry_video_driver->append_text("vdpau");
	combo_box_entry_video_driver->append_text("XDirectFB");
	combo_box_entry_video_driver->append_text("xshm");
	combo_box_entry_video_driver->append_text("xv");
	combo_box_entry_video_driver->append_text("xvmc");
	combo_box_entry_video_driver->append_text("xxmc");

	combo_box_entry_audio_driver->clear_items();
	combo_box_entry_audio_driver->append_text("null");
	combo_box_entry_audio_driver->append_text("pulseaudio");
	combo_box_entry_audio_driver->append_text("alsa");
	combo_box_entry_audio_driver->append_text("oss");
	combo_box_entry_audio_driver->append_text("esd");
	combo_box_entry_audio_driver->append_text("file");
	combo_box_entry_audio_driver->append_text("none");

	combo_box_entry_text_encoding->clear_items();
	combo_box_entry_text_encoding->append_text("auto");
	combo_box_entry_text_encoding->append_text("iso6937");

	combo_box_deinterlace_type->clear_items();
	combo_box_deinterlace_type->append_text("none");
	combo_box_deinterlace_type->append_text("standard");
	combo_box_deinterlace_type->append_text("tvtime");
	
	file_chooser_button_recording_directory->set_filename(configuration_manager.get_string_value("recording_directory"));
	spin_button_record_extra_before->set_value(configuration_manager.get_int_value("record_extra_before"));
	spin_button_record_extra_after->set_value(configuration_manager.get_int_value("record_extra_after"));
	spin_button_epg_span_hours->set_value(configuration_manager.get_int_value("epg_span_hours"));
	spin_button_epg_page_size->set_value(configuration_manager.get_int_value("epg_page_size"));
	spin_button_listen_port_webinterface->set_value(configuration_manager.get_int_value("listen_port_webinterface"));
	combo_box_entry_video_driver->get_entry()->set_text(configuration_manager.get_string_value("video_driver"));
	combo_box_entry_audio_driver->get_entry()->set_text(configuration_manager.get_string_value("audio_driver"));
	combo_box_deinterlace_type->set_active_text(configuration_manager.get_string_value("deinterlace_type"));
	combo_box_entry_text_encoding->get_entry()->set_text(configuration_manager.get_string_value("text_encoding"));
	check_button_keep_above->set_active(configuration_manager.get_boolean_value("keep_above"));
	check_button_show_epg_header->set_active(configuration_manager.get_boolean_value("show_epg_header"));
	check_button_show_epg_time->set_active(configuration_manager.get_boolean_value("show_epg_time"));
	check_button_show_epg_tooltips->set_active(configuration_manager.get_boolean_value("show_epg_tooltips"));
	check_button_display_status_icon->set_active(configuration_manager.get_boolean_value("display_status_icon"));
	check_button_show_channel_number->set_active(configuration_manager.get_boolean_value("show_channel_number"));
	check_button_remove_colon->set_active(configuration_manager.get_boolean_value("remove_colon"));
	check_button_enable_webinterface->set_active(configuration_manager.get_boolean_value("enable_webinterface"));
	
	if (Dialog::run() == Gtk::RESPONSE_OK)
	{
		configuration_manager.set_string_value("recording_directory", file_chooser_button_recording_directory->get_filename());
		configuration_manager.set_int_value("record_extra_before", (int)spin_button_record_extra_before->get_value());
		configuration_manager.set_int_value("record_extra_after", (int)spin_button_record_extra_after->get_value());
		configuration_manager.set_int_value("epg_span_hours", (int)spin_button_epg_span_hours->get_value());
		configuration_manager.set_int_value("epg_page_size", (int)spin_button_epg_page_size->get_value());
		configuration_manager.set_int_value("listen_port_webinterface", (int)spin_button_listen_port_webinterface->get_value());
		configuration_manager.set_string_value("video_driver", combo_box_entry_video_driver->get_entry()->get_text());
		configuration_manager.set_string_value("audio_driver", combo_box_entry_audio_driver->get_entry()->get_text());
		configuration_manager.set_string_value("deinterlace_type", combo_box_deinterlace_type->get_active_text());
		configuration_manager.set_string_value("text_encoding", combo_box_entry_text_encoding->get_entry()->get_text());
		configuration_manager.set_boolean_value("keep_above", check_button_keep_above->get_active());
		configuration_manager.set_boolean_value("show_epg_header", check_button_show_epg_header->get_active());
		configuration_manager.set_boolean_value("show_epg_time", check_button_show_epg_time->get_active());
		configuration_manager.set_boolean_value("show_epg_tooltips", check_button_show_epg_tooltips->get_active());
		configuration_manager.set_boolean_value("display_status_icon", check_button_display_status_icon->get_active());
		configuration_manager.set_boolean_value("show_channel_number", check_button_show_channel_number->get_active());
		configuration_manager.set_boolean_value("remove_colon", check_button_remove_colon->get_active());
		configuration_manager.set_boolean_value("enable_webinterface", check_button_enable_webinterface->get_active());
		
		signal_update();
	}
}
