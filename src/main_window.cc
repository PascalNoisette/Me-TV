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

#include "me-tv.h"
#include "me-tv-i18n.h"
#include "main_window.h"
#include "channels_dialog.h"
#include "devices_dialog.h"
#include "meters_dialog.h"
#include "preferences_dialog.h"
#include "application.h"
#include "scheduled_recordings_dialog.h"
#include "engine.h"
#include <libgnome/libgnome.h>
#include <gdk/gdkx.h>

#define POKE_INTERVAL 					30
#define UPDATE_INTERVAL					60
#define SECONDS_UNTIL_CHANNEL_CHANGE	3

MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
: Gtk::Window(cobject), builder(builder)
{
	g_debug("MainWindow constructor");

	g_static_rec_mutex_init(mutex.gobj());

	view_mode				= VIEW_MODE_EPG;
	prefullscreen_view_mode	= VIEW_MODE_EPG;
	last_update_time		= 0;
	last_poke_time			= 0;
	timeout_source			= 0;
	channel_change_timeout	= 0;
	temp_channel_number		= 0;
	engine					= NULL;
	output_fd				= -1;
	mute_state				= false;
	maximise_forced			= false;

	builder->get_widget("statusbar", statusbar);
	builder->get_widget("drawing_area_video", drawing_area_video);
	drawing_area_video->set_double_buffered(false);
	drawing_area_video->signal_expose_event().connect(sigc::mem_fun(*this, &MainWindow::on_drawing_area_expose_event));
	
	builder->get_widget_derived("scrolled_window_epg", widget_epg);
		
	Gtk::EventBox* event_box_video = NULL;
	builder->get_widget("event_box_video", event_box_video);
	event_box_video->signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_event_box_video_button_pressed));

	event_box_video->modify_fg(		Gtk::STATE_NORMAL, Gdk::Color("black"));
	drawing_area_video->modify_fg(	Gtk::STATE_NORMAL, Gdk::Color("black"));
	event_box_video->modify_bg(		Gtk::STATE_NORMAL, Gdk::Color("black"));
	drawing_area_video->modify_bg(	Gtk::STATE_NORMAL, Gdk::Color("black"));

	Gtk::AboutDialog* dialog_about = NULL;
	builder->get_widget("dialog_about", dialog_about);
	dialog_about->set_version(VERSION);

	is_cursor_visible = true;
	gchar     bits[] = {0};
	GdkColor  color = {0, 0, 0, 0};
	GdkPixmap* pixmap = gdk_bitmap_create_from_data(NULL, bits, 1, 1);
	hidden_cursor = gdk_cursor_new_from_pixmap(pixmap, pixmap, &color, &color, 0, 0);
	
	Application& application = get_application();
	set_keep_above(application.get_boolean_configuration_value("keep_above"));

	gtk_builder_connect_signals(builder->gobj(), NULL);
		
	ui_manager = Glib::RefPtr<Gtk::UIManager>::cast_dynamic(builder->get_object("ui_manager"));
	add_accel_group(ui_manager->get_accel_group());

	Gtk::Widget* menubar = ui_manager->get_widget("/menubar");
	Gtk::Widget* toolbar = ui_manager->get_widget("/toolbar");

	Gtk::VBox* vbox_main_window = NULL;
	builder->get_widget("vbox_main_window", vbox_main_window);

	vbox_main_window->pack_start(*menubar, Gtk::PACK_SHRINK);
	vbox_main_window->reorder_child(*menubar, 0);
	
	vbox_main_window->pack_start(*toolbar, Gtk::PACK_SHRINK);
	vbox_main_window->reorder_child(*toolbar, 1);

	menubar->show_all();
	set_view_mode(view_mode);

	last_motion_time = time(NULL);
	timeout_source = gdk_threads_add_timeout(1000, &MainWindow::on_timeout, this);

	g_debug("MainWindow constructed");
}

MainWindow::~MainWindow()
{
	stop_engine();
	if (timeout_source != 0)
	{
		g_source_remove(timeout_source);
	}
	save_geometry();
}

MainWindow* MainWindow::create(Glib::RefPtr<Gtk::Builder> builder)
{
	MainWindow* main_window = NULL;
	builder->get_widget_derived("window_main", main_window);
	return main_window;
}

void MainWindow::show_devices_dialog()
{
	FullscreenBugWorkaround fullscreen_bug_workaround;

	DevicesDialog& devices_dialog = DevicesDialog::create(builder);
	devices_dialog.run();
	devices_dialog.hide();
}

void MainWindow::show_channels_dialog()
{
	FullscreenBugWorkaround fullscreen_bug_workaround;

	ChannelsDialog& channels_dialog = ChannelsDialog::create(builder);	
	gint dialog_result = channels_dialog.run();
	channels_dialog.hide();

	if (dialog_result == Gtk::RESPONSE_OK)
	{
		const ChannelArray& channels = channels_dialog.get_channels();
		ChannelManager& channel_manager = get_application().channel_manager;
		channel_manager.set_channels(channels);
		get_application().select_channel_to_play();
	}
	update();
	
	Glib::RecMutex::Lock lock(mutex);
	if (engine == NULL)
	{
		start_engine();
	}
}

void MainWindow::show_preferences_dialog()
{
	FullscreenBugWorkaround fullscreen_bug_workaround;

	PreferencesDialog& preferences_dialog = PreferencesDialog::create(builder);
	preferences_dialog.run();
	preferences_dialog.hide();
	update();
}

void MainWindow::on_change_view_mode()
{
	if (view_mode == VIEW_MODE_VIDEO)
	{
		set_view_mode(VIEW_MODE_EPG);
	}
	else if (view_mode == VIEW_MODE_EPG)
	{
		set_view_mode(VIEW_MODE_CONTROLS);
	}
	else
	{
		set_view_mode(VIEW_MODE_VIDEO);
	}
	update();
}

bool MainWindow::on_delete_event(GdkEventAny* event)
{
	hide();
	return true;
}

bool MainWindow::on_event_box_video_button_pressed(GdkEventButton* event_button)
{
	TRY
	if (event_button->button == 1)
	{
		if (event_button->type == GDK_2BUTTON_PRESS)
		{
			Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(builder->get_object("fullscreen"))->activate();
		}
	}
	else if (event_button->button == 3)
	{
		on_change_view_mode();
	}
	CATCH
	
	return false;
}

bool MainWindow::on_motion_notify_event(GdkEventMotion* event_motion)
{
	last_motion_time = time(NULL);
	if (!is_cursor_visible)
	{
		Gtk::Widget* widget = NULL;
		builder->get_widget("event_box_video", widget);
		Glib::RefPtr<Gdk::Window> event_box_video = widget->get_window();
		if (event_box_video)
		{
			event_box_video->set_cursor();
			is_cursor_visible = true;
		}
	}
	return true;
}


void MainWindow::unfullscreen(gboolean restore_mode)
{
	Gtk::Window::unfullscreen();
	
	if (restore_mode)
	{
		set_view_mode(prefullscreen_view_mode);
	}
}

void MainWindow::fullscreen(gboolean change_mode)
{
	prefullscreen_view_mode = view_mode;
	if (change_mode)
	{
		set_view_mode(VIEW_MODE_VIDEO);
	}
	
	Gtk::Window::fullscreen();
}

gboolean MainWindow::is_fullscreen()
{
	return get_window()->get_state() & Gdk::WINDOW_STATE_FULLSCREEN;
}

gboolean MainWindow::on_timeout(gpointer data)
{
	MainWindow* main_window = (MainWindow*)data;
	main_window->on_timeout();
	return true;
}

void MainWindow::on_timeout()
{
	static gboolean poke_failed = false;
	
	TRY
	guint now = time(NULL);
	
	if (channel_change_timeout > 1)
	{
		channel_change_timeout--;
	}
	else if (channel_change_timeout == 1)
	{
		// Deactivate the countdown
		channel_change_timeout = 0;

		guint channel_index = temp_channel_number - 1;

		// Reset the temporary channel number for the next run
		temp_channel_number = 0;		

		get_application().set_display_channel_number(channel_index);
	}
	
	// Hide the mouse
	if (now - last_motion_time > 3 && is_cursor_visible)
	{
		Gtk::Widget* widget = NULL;
		builder->get_widget("event_box_video", widget);
		Glib::RefPtr<Gdk::Window> event_box_video = widget->get_window();
		if (event_box_video)
		{
			event_box_video->set_cursor(Gdk::Cursor(hidden_cursor));
			is_cursor_visible = false;
		}
	}
	
	// Update EPG
	guint last_epg_update_time = get_application().stream_manager.get_last_epg_update_time();
	if ((last_epg_update_time > last_update_time) || (now - last_update_time > UPDATE_INTERVAL))
	{
		update();
		last_update_time = now;
	}
	
	// Check on engine
	if (engine != NULL && !engine->is_running())
	{
		stop_engine();
	}
	
	// Disable screensaver
	if (now - last_poke_time > POKE_INTERVAL && !poke_failed)
	{
		Glib::RefPtr<Gdk::Window> window = get_window();
		gboolean is_minimised = window == NULL || window->get_state() & Gdk::WINDOW_STATE_ICONIFIED;
		if (is_visible() && !is_minimised)
		{
			try
			{
				Glib::ustring screensaver_poke_command = get_application().get_string_configuration_value("screensaver_poke_command");
				if (!screensaver_poke_command.empty())
				{
					g_debug("Poking screensaver");
					Glib::spawn_command_line_async(screensaver_poke_command);
				}
			}
			catch(...)
			{
				poke_failed = true;
				throw Exception(_("Failed to poke screensaver"));
			}
		}

		last_poke_time = now;
	}
	
	CATCH
}

void MainWindow::pause(gboolean state)
{
	if (engine != NULL)
	{
		engine->pause(state);
	}
}

void MainWindow::set_view_mode(ViewMode mode)
{
	Gtk::Widget* widget = NULL;

	widget = ui_manager->get_widget("/menubar");
	widget->property_visible() = (mode == VIEW_MODE_EPG) || (mode == VIEW_MODE_CONTROLS);
	
	widget = ui_manager->get_widget("/toolbar");
	widget->property_visible() = (mode == VIEW_MODE_EPG) || (mode == VIEW_MODE_CONTROLS);
	
	builder->get_widget("statusbar", widget);
	widget->property_visible() = (mode == VIEW_MODE_EPG) || (mode == VIEW_MODE_CONTROLS);
	
	builder->get_widget("vbox_epg", widget);
	widget->property_visible() = (mode == VIEW_MODE_EPG);
	
	view_mode = mode;
}

void MainWindow::show_scheduled_recordings_dialog()
{
	FullscreenBugWorkaround fullscreen_bug_workaround;

	ScheduledRecordingsDialog& scheduled_recordings_dialog = ScheduledRecordingsDialog::create(builder);
	scheduled_recordings_dialog.run();
	scheduled_recordings_dialog.hide();
}

void MainWindow::on_menu_item_audio_stream_activate(guint index)
{
	Glib::RecMutex::Lock lock(mutex);

	g_debug("MainWindow::on_menu_item_audio_stream_activate(%d)", index);
	if (engine != NULL)
	{
		engine->set_audio_stream(index);
	}
}

void MainWindow::on_menu_item_subtitle_stream_activate(guint index)
{
	Glib::RecMutex::Lock lock(mutex);

	g_debug("MainWindow::on_menu_item_subtitle_stream_activate(%d)", index);
	if (engine != NULL)
	{
		engine->set_subtitle_stream(index);
	}
}

void MainWindow::update()
{	
	Application& application = get_application();
	Glib::ustring window_title;
	Glib::ustring status_text;

	if (!application.channel_manager.has_display_channel())
	{
		window_title = _("Me TV - It's TV for me computer");
	}
	else
	{
		Channel& channel = application.channel_manager.get_display_channel();
		window_title = "Me TV - " + channel.get_text();
		status_text = channel.get_text();
	}

	
	if (application.stream_manager.is_recording())
	{
		status_text += _(" [Recording]");
		window_title += _(" [Recording]");
	}
	
	set_title(window_title);
	statusbar->pop();
	statusbar->push(status_text);

	widget_epg->update();
}

void MainWindow::on_show()
{
	Application& application = get_application();
	
	gint x = application.get_int_configuration_value("x");
	gint y = application.get_int_configuration_value("y");
	gint width = application.get_int_configuration_value("width");
	gint height = application.get_int_configuration_value("height");
		
	if (x != -1)
	{
		g_debug("Setting geometry (%d, %d, %d, %d)", x, y, width, height);
		move(x, y);
		set_default_size(width, height);
	}

	Gtk::Window::on_show();
	Gdk::Window::process_all_updates();

	TRY
	start_engine();
	if (application.get_boolean_configuration_value("keep_above"))
	{
		set_keep_above();
	}
	Gtk::EventBox* event_box_video = NULL;
	builder->get_widget("event_box_video", event_box_video);
	event_box_video->resize_children();
	update();

	CATCH
}

void MainWindow::on_hide()
{
	save_geometry();

	TRY
	stop_engine();
	Gtk::Window::on_hide();
	if (!get_application().get_boolean_configuration_value("display_status_icon"))
	{
		Gnome::Main::quit();
	}
	CATCH
}

void MainWindow::save_geometry()
{
	if (property_visible())
	{
		gint x = -1;
		gint y = -1;
		gint width = -1;
		gint height = -1;
		
		get_position(x, y);
		get_size(width, height);
		
		Application& application = get_application();
		application.set_int_configuration_value("x", x);
		application.set_int_configuration_value("y", y);
		application.set_int_configuration_value("width", width);
		application.set_int_configuration_value("height", height);

		g_debug("Saved geometry (%d, %d, %d, %d)", x, y, width, height);
	}
}

void MainWindow::toggle_visibility()
{
	property_visible() = !property_visible();
}

void MainWindow::add_channel_number(guint channel_number)
{
	g_debug("Key %d pressed", channel_number);

	temp_channel_number *= 10;
	temp_channel_number += channel_number;

	if (channel_change_timeout == 0)
	{
		channel_change_timeout = SECONDS_UNTIL_CHANNEL_CHANGE;
	}
}

bool MainWindow::on_key_press_event(GdkEventKey* event_key)
{
	switch(event_key->keyval)
	{		
		case GDK_0: add_channel_number(0); break;
		case GDK_1: add_channel_number(1); break;
		case GDK_2: add_channel_number(2); break;
		case GDK_3: add_channel_number(3); break;
		case GDK_4: add_channel_number(4); break;
		case GDK_5: add_channel_number(5); break;
		case GDK_6: add_channel_number(6); break;
		case GDK_7: add_channel_number(7); break;
		case GDK_8: add_channel_number(8); break;
		case GDK_9: add_channel_number(9); break;
			
 		default:
			break;
	}
	
	return Gtk::Window::on_key_press_event(event_key);
}

bool MainWindow::on_drawing_area_expose_event(GdkEventExpose* event_expose)
{
	TRY
	if (engine == NULL)
	{
		drawing_area_video->get_window()->draw_rectangle(
			drawing_area_video->get_style()->get_bg_gc(Gtk::STATE_NORMAL), true,
			event_expose->area.x, event_expose->area.y,
			event_expose->area.width, event_expose->area.height);
	}
	CATCH

	return false;
}

void MainWindow::create_engine()
{
	if (engine != NULL)
	{
		throw Exception(_("Failed to start engine: Engine has already been started"));
	}
	
	g_debug("Creating engine");
	Application& application = get_application();
	Glib::ustring engine_type = application.get_string_configuration_value("engine_type");
	if (engine_type == "none")
	{
		engine = NULL;
	}
	else
	{
		engine = new Engine(engine_type);
		engine->set_mute_state(mute_state);
	}
	
	g_debug("%s engine created", engine_type.c_str());
}

void MainWindow::play(const Glib::ustring& mrl)
{
	Application& application = get_application();
	
	if (engine == NULL)
	{
		create_engine();
		if (engine != NULL)
		{
			engine->play(mrl);
		}
	}
	
	Gtk::Menu* audio_streams_menu = ((Gtk::MenuItem*)ui_manager->get_widget("/menubar/audio/audio_streams"))->get_submenu();
	Gtk::Menu* subtitle_streams_menu = ((Gtk::MenuItem*)ui_manager->get_widget("/menubar/video/subtitle_streams"))->get_submenu();
	
	Gtk::Menu_Helpers::MenuList& audio_items = audio_streams_menu->items();
	audio_items.erase(audio_items.begin(), audio_items.end());

	Gtk::Menu_Helpers::MenuList& subtitle_items = subtitle_streams_menu->items();
	subtitle_items.erase(subtitle_items.begin(), subtitle_items.end());

	Gtk::RadioMenuItem::Group audio_streams_menu_group;

	// TODO: Generate menus
	// Acquire stream thread lock
/*	Glib::RecMutex::Lock application_lock(application.get_mutex());

	StreamThread* stream_thread = application.get_stream_thread();
	if (stream_thread != NULL)
	{
		const Mpeg::Stream& stream = stream_thread->get_stream();
		std::vector<Mpeg::AudioStream> audio_streams = stream.audio_streams;
		guint count = 0;
		
		g_debug("Audio streams: %zu", audio_streams.size());
		for (std::vector<Mpeg::AudioStream>::iterator i = audio_streams.begin(); i != audio_streams.end(); i++)
		{
			Mpeg::AudioStream audio_stream = *i;
			Glib::ustring text = Glib::ustring::compose("%1: %2", count, audio_stream.language);
			if (audio_stream.is_ac3)
			{
				text += " (AC3)";
			}
			Gtk::RadioMenuItem* menu_item = new Gtk::RadioMenuItem(audio_streams_menu_group, text);
			menu_item->show_all();
			audio_streams_menu->items().push_back(*menu_item);
			
			menu_item->signal_activate().connect(
				sigc::bind<guint>
				(
					sigc::mem_fun(*this, &MainWindow::on_menu_item_audio_stream_activate),
					count
				)
			);

			count++;
		}

		std::vector<Mpeg::SubtitleStream> subtitle_streams = stream.subtitle_streams;
		Gtk::RadioMenuItem::Group subtitle_streams_menu_group;
		count = 0;
		
		Gtk::RadioMenuItem* menu_item_subtitle_none = new Gtk::RadioMenuItem(subtitle_streams_menu_group, _("None"));
		menu_item_subtitle_none->show_all();
		subtitle_streams_menu->items().push_back(*menu_item_subtitle_none);
		menu_item_subtitle_none->signal_activate().connect(
			sigc::bind<guint>
			(
				sigc::mem_fun(*this, &MainWindow::on_menu_item_subtitle_stream_activate),
				-1
			)
		);
		
		g_debug("Subtitle streams: %zu", subtitle_streams.size());
		for (std::vector<Mpeg::SubtitleStream>::iterator i = subtitle_streams.begin(); i != subtitle_streams.end(); i++)
		{
			Mpeg::SubtitleStream subtitle_stream = *i;
			Glib::ustring text = Glib::ustring::compose("%1: %2", count, subtitle_stream.language);
			Gtk::RadioMenuItem* menu_item = new Gtk::RadioMenuItem(subtitle_streams_menu_group, text);
			menu_item->show_all();
			subtitle_streams_menu->items().push_back(*menu_item);
						
			menu_item->signal_activate().connect(
				sigc::bind<guint>
				(
					sigc::mem_fun(*this, &MainWindow::on_menu_item_subtitle_stream_activate),
					count
				)
			);
			
			count++;
		}
	}
	*/
	if (audio_streams_menu->items().empty())
	{
		Gtk::MenuItem* menu_item = new Gtk::MenuItem(_("Not available"));
		menu_item->show_all();
		audio_streams_menu->items().push_back(*menu_item);
	}

	if (subtitle_streams_menu->items().empty())
	{
		Gtk::MenuItem* menu_item = new Gtk::MenuItem(_("Not available"));
		menu_item->show_all();
		subtitle_streams_menu->items().push_back(*menu_item);
	}
}

void MainWindow::start_engine()
{
	Glib::RecMutex::Lock lock(mutex);

	if (property_visible())
	{
		play(get_application().stream_manager.get_display_stream().filename);
	}
}

void MainWindow::stop_engine()
{
	g_debug("Stopping engine");

	Glib::RecMutex::Lock lock(mutex);
	if (engine != NULL)
	{
		engine->stop();
		delete engine;
		engine = NULL;
	}

	g_debug("Engine stopped");
}

void MainWindow::on_audio_channel_both()
{
	if (engine != NULL)
	{
		engine->set_audio_channel_state(Engine::AUDIO_CHANNEL_STATE_BOTH);
	}
}

void MainWindow::on_audio_channel_left()
{
	if (engine != NULL)
	{
		engine->set_audio_channel_state(Engine::AUDIO_CHANNEL_STATE_LEFT);
	}
}

void MainWindow::on_audio_channel_right()
{
	if (engine != NULL)
	{
		engine->set_audio_channel_state(Engine::AUDIO_CHANNEL_STATE_RIGHT);
	}
}

void MainWindow::on_devices()
{
	TRY
	show_devices_dialog();
	CATCH
}

void MainWindow::on_channels()
{
	TRY
	show_channels_dialog();
	CATCH
}

void MainWindow::on_scheduled_recordings()
{
	TRY
	show_scheduled_recordings_dialog();
	CATCH
}

void MainWindow::on_meters()
{
	TRY	
	// Check that there is a device
	get_application().device_manager.get_frontend();
	
	MetersDialog& meters_dialog = MetersDialog::create(builder);
	meters_dialog.stop();
	meters_dialog.start();
	meters_dialog.show();
	CATCH
}

void MainWindow::on_preferences()
{
	TRY
	show_preferences_dialog();
	CATCH
}

void MainWindow::on_about()
{
	TRY
	FullscreenBugWorkaround fullscreen_bug_workaround;

	Gtk::Dialog* about_dialog = NULL;
	builder->get_widget("dialog_about", about_dialog);
	about_dialog->run();
	about_dialog->hide();
	CATCH
}

void MainWindow::on_mute()
{
	TRY
	Gtk::ToggleToolButton* mute_button =
		(Gtk::ToggleToolButton*)ui_manager->get_widget("/toolbar/mute");
	set_mute_state(mute_button->get_active());
	CATCH
}

void MainWindow::set_mute_state(gboolean state)
{
	mute_state = state;
	if (engine != NULL)
	{
		engine->set_mute_state(mute_state);
	}
}

void MainWindow::toggle_fullscreen()
{
	if (is_fullscreen())
	{
		unfullscreen();

		if (maximise_forced)
		{
			get_window()->unmaximize();
			maximise_forced = false;
		}
	}
	else
	{
		if (get_application().get_boolean_configuration_value("fullscreen_bug_workaround"))
		{
			maximise_forced = true;
			get_window()->maximize();
		}	

		fullscreen();
	}
}

void MainWindow::on_fullscreen()
{
	toggle_fullscreen();
}
