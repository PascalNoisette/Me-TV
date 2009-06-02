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
	Application& application = get_application();
	
	Gtk::FileChooserButton* file_chooser_button_recording_directory = NULL;
	Gtk::SpinButton* spin_button_record_extra_before = NULL;
	Gtk::SpinButton* spin_button_record_extra_after = NULL;
	Gtk::SpinButton* spin_button_epg_span_hours = NULL;
	Gtk::SpinButton* spin_button_epg_page_size = NULL;
	Gtk::Entry* entry_broadcast_address = NULL;
	Gtk::SpinButton* spin_button_broadcast_port = NULL;
	ComboBoxEntryText* combo_box_entry_preferred_language = NULL;
	ComboBoxEntryText* combo_box_entry_xine_video_driver = NULL;
	ComboBoxEntryText* combo_box_entry_xine_audio_driver = NULL;
	ComboBoxEntryText* combo_box_entry_text_encoding = NULL;
	Gtk::CheckButton* check_button_keep_above = NULL;
	Gtk::CheckButton* check_button_show_epg_header = NULL;
	Gtk::CheckButton* check_button_show_epg_time = NULL;
	Gtk::CheckButton* check_button_show_epg_tooltips = NULL;
	Gtk::CheckButton* check_button_24_hour_workaround = NULL;
	Gtk::CheckButton* check_button_fullscreen_bug_workaround = NULL;
	Gtk::CheckButton* check_button_display_status_icon = NULL;

	builder->get_widget("file_chooser_button_recording_directory", file_chooser_button_recording_directory);
	builder->get_widget("spin_button_record_extra_before", spin_button_record_extra_before);
	builder->get_widget("spin_button_record_extra_after", spin_button_record_extra_after);
	builder->get_widget("spin_button_epg_span_hours", spin_button_epg_span_hours);
	builder->get_widget("spin_button_epg_page_size", spin_button_epg_page_size);
	builder->get_widget("entry_broadcast_address", entry_broadcast_address);
	builder->get_widget("spin_button_broadcast_port", spin_button_broadcast_port);
	builder->get_widget_derived("combo_box_entry_preferred_language", combo_box_entry_preferred_language);
	builder->get_widget_derived("combo_box_entry_xine_video_driver", combo_box_entry_xine_video_driver);
	builder->get_widget_derived("combo_box_entry_xine_audio_driver", combo_box_entry_xine_audio_driver);
	builder->get_widget_derived("combo_box_entry_text_encoding", combo_box_entry_text_encoding);
	builder->get_widget("check_button_keep_above", check_button_keep_above);
	builder->get_widget("check_button_show_epg_header", check_button_show_epg_header);
	builder->get_widget("check_button_show_epg_time", check_button_show_epg_time);
	builder->get_widget("check_button_show_epg_tooltips", check_button_show_epg_tooltips);
	builder->get_widget("check_button_24_hour_workaround", check_button_24_hour_workaround);
	builder->get_widget("check_button_fullscreen_bug_workaround", check_button_fullscreen_bug_workaround);
	builder->get_widget("check_button_display_status_icon", check_button_display_status_icon);

	ComboBoxText* combo_box_engine_type = NULL;
	builder->get_widget_derived("combo_box_engine_type", combo_box_engine_type);
	
	combo_box_engine_type->clear_items();
	combo_box_engine_type->append_text("none");
#ifdef ENABLE_XINE_ENGINE
	combo_box_engine_type->append_text("xine");
#endif
#ifdef ENABLE_LIBGSTREAMER_ENGINE
	combo_box_engine_type->append_text("libgstreamer");
#endif
#ifdef ENABLE_XINE_LIB_ENGINE
	combo_box_engine_type->append_text("xine-lib");
#endif
#ifdef ENABLE_LIBVLC_ENGINE
	combo_box_engine_type->append_text("libvlc");
#endif
#ifdef ENABLE_MPLAYER_ENGINE
	combo_box_engine_type->append_text("mplayer");
#endif

	combo_box_entry_preferred_language->append_text("eng");
	combo_box_entry_preferred_language->append_text("fin");
	combo_box_entry_preferred_language->append_text("ger");
	combo_box_entry_preferred_language->append_text("swe");
	combo_box_entry_preferred_language->append_text("fre");
	
	combo_box_entry_xine_video_driver->append_text("dxr3");
	combo_box_entry_xine_video_driver->append_text("aadxr3");
	combo_box_entry_xine_video_driver->append_text("xv");
	combo_box_entry_xine_video_driver->append_text("XDirectFB");
	combo_box_entry_xine_video_driver->append_text("DirectFB");
	combo_box_entry_xine_video_driver->append_text("SyncFB");
	combo_box_entry_xine_video_driver->append_text("opengl");
	combo_box_entry_xine_video_driver->append_text("xshm");
	combo_box_entry_xine_video_driver->append_text("none");
	combo_box_entry_xine_video_driver->append_text("xxmc");
	combo_box_entry_xine_video_driver->append_text("sdl");
	combo_box_entry_xine_video_driver->append_text("fb");
	combo_box_entry_xine_video_driver->append_text("xvmc");

	combo_box_entry_xine_audio_driver->append_text("null");
	combo_box_entry_xine_audio_driver->append_text("pulseaudio");
	combo_box_entry_xine_audio_driver->append_text("alsa");
	combo_box_entry_xine_audio_driver->append_text("oss");
	combo_box_entry_xine_audio_driver->append_text("esd");
	combo_box_entry_xine_audio_driver->append_text("file");
	combo_box_entry_xine_audio_driver->append_text("none");

	combo_box_entry_text_encoding->append_text("auto");
	combo_box_entry_text_encoding->append_text("iso6937");
	
	file_chooser_button_recording_directory->set_filename(application.get_string_configuration_value("recording_directory"));
	spin_button_record_extra_before->set_value(application.get_int_configuration_value("record_extra_before"));
	spin_button_record_extra_after->set_value(application.get_int_configuration_value("record_extra_after"));
	spin_button_epg_span_hours->set_value(application.get_int_configuration_value("epg_span_hours"));
	spin_button_epg_page_size->set_value(application.get_int_configuration_value("epg_page_size"));
	entry_broadcast_address->set_text(application.get_string_configuration_value("broadcast_address"));
	spin_button_broadcast_port->set_value(application.get_int_configuration_value("broadcast_port"));
	combo_box_engine_type->set_active_text(application.get_string_configuration_value("engine_type"));
	combo_box_entry_preferred_language->get_entry()->set_text(application.get_string_configuration_value("preferred_language"));
	combo_box_entry_xine_video_driver->get_entry()->set_text(application.get_string_configuration_value("xine.video_driver"));
	combo_box_entry_xine_audio_driver->get_entry()->set_text(application.get_string_configuration_value("xine.audio_driver"));
	combo_box_entry_text_encoding->get_entry()->set_text(application.get_string_configuration_value("text_encoding"));
	check_button_keep_above->set_active(application.get_boolean_configuration_value("keep_above"));
	check_button_show_epg_header->set_active(application.get_boolean_configuration_value("show_epg_header"));
	check_button_show_epg_time->set_active(application.get_boolean_configuration_value("show_epg_time"));
	check_button_show_epg_tooltips->set_active(application.get_boolean_configuration_value("show_epg_tooltips"));
	check_button_24_hour_workaround->set_active(application.get_boolean_configuration_value("use_24_hour_workaround"));
	check_button_fullscreen_bug_workaround->set_active(application.get_boolean_configuration_value("fullscreen_bug_workaround"));
	check_button_display_status_icon->set_active(application.get_boolean_configuration_value("display_status_icon"));
	
	if (Dialog::run() == Gtk::RESPONSE_OK)
	{
		application.set_string_configuration_value("recording_directory", file_chooser_button_recording_directory->get_filename());
		application.set_int_configuration_value("record_extra_before", (int)spin_button_record_extra_before->get_value());
		application.set_int_configuration_value("record_extra_after", (int)spin_button_record_extra_after->get_value());
		application.set_int_configuration_value("epg_span_hours", (int)spin_button_epg_span_hours->get_value());
		application.set_int_configuration_value("epg_page_size", (int)spin_button_epg_page_size->get_value());
		application.set_string_configuration_value("broadcast_address", entry_broadcast_address->get_text());
		application.set_int_configuration_value("broadcast_port", (int)spin_button_broadcast_port->get_value());
		application.set_string_configuration_value("preferred_language", combo_box_entry_preferred_language->get_active_text());
		application.set_string_configuration_value("engine_type", combo_box_engine_type->get_active_text());
		application.set_string_configuration_value("xine.video_driver", combo_box_entry_xine_video_driver->get_active_text());
		application.set_string_configuration_value("xine.audio_driver", combo_box_entry_xine_audio_driver->get_active_text());
		application.set_string_configuration_value("text_encoding", combo_box_entry_text_encoding->get_active_text());
		application.set_boolean_configuration_value("keep_above", check_button_keep_above->get_active());
		application.set_boolean_configuration_value("show_epg_header", check_button_show_epg_header->get_active());
		application.set_boolean_configuration_value("show_epg_time", check_button_show_epg_time->get_active());
		application.set_boolean_configuration_value("show_epg_tooltips", check_button_show_epg_tooltips->get_active());
		application.set_boolean_configuration_value("use_24_hour_workaround", check_button_24_hour_workaround->get_active());
		application.set_boolean_configuration_value("fullscreen_bug_workaround", check_button_fullscreen_bug_workaround->get_active());
		application.set_boolean_configuration_value("display_status_icon", check_button_display_status_icon->get_active());

		get_application().update();
	}
}
