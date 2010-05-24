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

#include "me-tv.h"
#include "me-tv-i18n.h"
#include "application.h"
#include "main_window.h"
#include "channels_dialog.h"
#include "preferences_dialog.h"
#include "scheduled_recordings_dialog.h"
#include "epg_event_search_dialog.h"
#include "engine.h"
#include <gtkmm.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#define UPDATE_INTERVAL					60
#define SECONDS_UNTIL_CHANNEL_CHANGE	3
#define GS_SERVICE   					"org.gnome.ScreenSaver"
#define GS_PATH      					"/org/gnome/ScreenSaver"
#define GS_INTERFACE 					"org.gnome.ScreenSaver"

Glib::ustring ui_info =
	"<ui>"
	"	<menubar name='menu_bar'>"
	"		<menu action='action_file'>"
	"			<menuitem action='toggle_action_record'/>"
	"			<separator/>"
	"			<menuitem action='action_quit'/>"
	"		</menu>"
	"		<menu action='action_view'>"
	"			<menuitem action='action_previous_channel'/>"
	"			<menuitem action='action_next_channel'/>"
	"			<separator/>"
	"			<menuitem action='action_change_view_mode'/>"
	"			<separator/>"
	"			<menuitem action='action_scheduled_recordings'/>"
	"			<menuitem action='action_epg_event_search'/>"
	"			<menuitem action='action_channels'/>"
	"			<menuitem action='action_preferences'/>"
	"		</menu>"
	"		<menu action='action_video'>"
	"			<menuitem action='toggle_action_fullscreen'/>"
	"			<menu action='action_subtitle_streams'>"
	"			</menu>"
	"		</menu>"
	"		<menu action='action_audio'>"
	"			<menuitem action='toggle_action_mute'/>"
	"			<menu action='action_audio_streams'>"
	"			</menu>"
	"			<menu action='action_audio_channels'>"
	"				<menuitem action='action_audio_channel_both'/>"
	"				<menuitem action='action_audio_channel_left'/>"
	"				<menuitem action='action_audio_channel_right'/>"
	"			</menu>"
	"		</menu>"
	"		<menu action='action_help'>"
	"			<menuitem action='action_about'/>"
	"		</menu>"
	"	</menubar>"
	"	<toolbar name='toolbar'>"
	"		<toolitem action='toggle_action_record'/>"
	"		<toolitem action='toggle_action_mute'/>"
	"		<toolitem action='action_scheduled_recordings'/>"
	"		<toolitem action='action_epg_event_search'/>"
	"		<toolitem action='toggle_action_fullscreen'/>"
	"	</toolbar>"
	"</ui>";

MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
: Gtk::Window(cobject), builder(builder)
{
	g_debug("MainWindow constructor");

	view_mode				= VIEW_MODE_EPG;
	prefullscreen_view_mode	= VIEW_MODE_EPG;
	last_update_time		= 0;
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
	
	add_accel_group(ui_manager->get_accel_group());
	ui_manager->add_ui_from_string(ui_info);

	Gtk::Widget* menu_bar = (Gtk::MenuBar*)ui_manager->get_widget("/menu_bar");
	Gtk::Toolbar* toolbar = (Gtk::Toolbar*)ui_manager->get_widget("/toolbar");

	Gtk::VBox* vbox_main_window = NULL;
	builder->get_widget("vbox_main_window", vbox_main_window);

	vbox_main_window->pack_start(*menu_bar, Gtk::PACK_SHRINK);
	vbox_main_window->reorder_child(*menu_bar, 0);
	
	vbox_main_window->pack_start(*toolbar, Gtk::PACK_SHRINK);
	vbox_main_window->reorder_child(*toolbar, 1);

	menu_bar->show_all();
	set_view_mode(view_mode);

	connection_exception = Glib::add_exception_handler(sigc::mem_fun(*this, &MainWindow::on_exception));

	toggle_action_fullscreen->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_fullscreen));
	toggle_action_mute->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_mute));

	action_about->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_about));
	action_channels->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::show_channels_dialog));
	action_change_view_mode->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_change_view_mode));
	action_epg_event_search->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_epg_event_search));
	action_preferences->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_preferences));
	action_scheduled_recordings->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_scheduled_recordings));
	
	dbus_error_init (&dbus_error);
	dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
	if (dbus_connection == NULL)
	{
		g_message(_("Failed to connect to the D-BUS daemon: %s"), dbus_error.message);
		dbus_error_free(&dbus_error);
	}

	last_motion_time = time(NULL);
	timeout_source = gdk_threads_add_timeout(1000, &MainWindow::on_timeout, this);

	g_debug("MainWindow constructed");
}

MainWindow::~MainWindow()
{
	connection_exception.disconnect();
	stop_engine();
	if (timeout_source != 0)
	{
		g_source_remove(timeout_source);
	}
	save_geometry();

	g_debug("MainWindow destroyed");
}

MainWindow* MainWindow::create(Glib::RefPtr<Gtk::Builder> builder)
{
	MainWindow* main_window = NULL;
	builder->get_widget_derived("window_main", main_window);
	return main_window;
}

void MainWindow::show_channels_dialog()
{
	FullscreenBugWorkaround fullscreen_bug_workaround;

	if (get_application().stream_manager.is_recording())
	{
		throw Exception(_("Please stop all recordings before editing channels"));
	}
	
	ChannelsDialog& channels_dialog = ChannelsDialog::create(builder);	
	gint dialog_result = channels_dialog.run();
	channels_dialog.hide();

	if (dialog_result == Gtk::RESPONSE_OK)
	{
		if (get_application().stream_manager.is_recording())
		{
			throw Exception(_("Cannot update channels while recording"));
		}

		const ChannelArray& channels = channels_dialog.get_channels();
		get_application().stream_manager.stop();
		get_application().channel_manager.set_channels(channels);
		get_application().stream_manager.start();
		get_application().select_channel_to_play();

		StreamManager& stream_manager = get_application().stream_manager;
		if (stream_manager.has_display_stream())
		{
			get_application().set_display_channel(stream_manager.get_display_channel());
		}
	}
	update();
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
	if (event_button->button == 1)
	{
		if (event_button->type == GDK_2BUTTON_PRESS)
		{
			toggle_action_fullscreen->activate();
		}
	}
	else if (event_button->button == 3)
	{
		on_change_view_mode();
	}
	
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
	return get_window() != NULL && get_window()->get_state() & Gdk::WINDOW_STATE_FULLSCREEN;
}

gboolean MainWindow::on_timeout(gpointer data)
{
	MainWindow* main_window = (MainWindow*)data;
	main_window->on_timeout();
	return true;
}

void MainWindow::on_timeout()
{
	try
	{
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
	}
	catch(...)
	{
		on_exception();
	}
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

	widget = ui_manager->get_widget("/menu_bar");
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

	update();
}

void MainWindow::show_epg_event_search_dialog()
{
	FullscreenBugWorkaround fullscreen_bug_workaround;

	EpgEventSearchDialog& epg_event_search_dialog = EpgEventSearchDialog::get(builder);
	epg_event_search_dialog.run();
	epg_event_search_dialog.hide();

	update();
}

void MainWindow::on_menu_item_audio_stream_activate(guint index)
{
	g_debug("MainWindow::on_menu_item_audio_stream_activate(%d)", index);
	if (engine != NULL)
	{
		engine->set_audio_stream(index);
	}
}

void MainWindow::on_menu_item_subtitle_stream_activate(guint index)
{
	g_debug("MainWindow::on_menu_item_subtitle_stream_activate(%d)", index);
	if (engine != NULL)
	{
		engine->set_subtitle_stream(index);
	}
}

void MainWindow::update()
{	
	Application& application = get_application();
	Glib::ustring status_text;
	Glib::ustring window_title = "Me TV - It's TV for me computer";

	if (application.stream_manager.has_display_stream())
	{
		Channel& channel = application.stream_manager.get_display_channel();
		window_title = "Me TV - " + channel.get_text();
		status_text = channel.get_text();
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
		
	if (width > 0 && height > 0)
	{
		g_debug("Setting geometry (%d, %d, %d, %d)", x, y, width, height);
		move(x, y);
		set_default_size(width, height);
	}

	Gtk::Window::on_show();
	Gdk::Window::process_all_updates();

	if (get_application().stream_manager.has_display_stream())
	{
		start_engine();
	}
	
	if (application.get_boolean_configuration_value("keep_above"))
	{
		set_keep_above();
	}
	
	Gtk::EventBox* event_box_video = NULL;
	builder->get_widget("event_box_video", event_box_video);
	event_box_video->resize_children();
	update();
}

void MainWindow::on_hide()
{
	save_geometry();

	stop_engine();
	Gtk::Window::on_hide();
	if (!get_application().get_boolean_configuration_value("display_status_icon"))
	{
		Gtk::Main::quit();
	}
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
		case GDK_KP_0: case GDK_0: add_channel_number(0); break;
		case GDK_KP_1: case GDK_1: add_channel_number(1); break;
		case GDK_KP_2: case GDK_2: add_channel_number(2); break;
		case GDK_KP_3: case GDK_3: add_channel_number(3); break;
		case GDK_KP_4: case GDK_4: add_channel_number(4); break;
		case GDK_KP_5: case GDK_5: add_channel_number(5); break;
		case GDK_KP_6: case GDK_6: add_channel_number(6); break;
		case GDK_KP_7: case GDK_7: add_channel_number(7); break;
		case GDK_KP_8: case GDK_8: add_channel_number(8); break;
		case GDK_KP_9: case GDK_9: add_channel_number(9); break;
			
 		default:
			break;
	}
	
	return Gtk::Window::on_key_press_event(event_key);
}

bool MainWindow::on_drawing_area_expose_event(GdkEventExpose* event_expose)
{
	if (engine == NULL)
	{
		drawing_area_video->get_window()->draw_rectangle(
			drawing_area_video->get_style()->get_bg_gc(Gtk::STATE_NORMAL), true,
			event_expose->area.x, event_expose->area.y,
			event_expose->area.width, event_expose->area.height);
	}

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
	engine = new Engine();
	engine->set_mute_state(mute_state);
	
	g_debug("Engine created");
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
			inhibit_screensaver(true);
		}
	}

	Glib::ustring preferred_language = application.get_preferred_language();

	if (!preferred_language.empty())
	{
		g_debug("*** Preferred language has been set, waiting 1 second for player to start");
		usleep(1000000);
	}
	
	Gtk::Menu* audio_streams_menu = ((Gtk::MenuItem*)ui_manager->get_widget("/menu_bar/action_audio/action_audio_streams"))->get_submenu();
	Gtk::Menu* subtitle_streams_menu = ((Gtk::MenuItem*)ui_manager->get_widget("/menu_bar/action_video/action_subtitle_streams"))->get_submenu();
	
	Gtk::Menu_Helpers::MenuList& audio_items = audio_streams_menu->items();
	audio_items.erase(audio_items.begin(), audio_items.end());

	Gtk::Menu_Helpers::MenuList& subtitle_items = subtitle_streams_menu->items();
	subtitle_items.erase(subtitle_items.begin(), subtitle_items.end());

	Gtk::RadioMenuItem::Group audio_streams_menu_group;

	Mpeg::Stream& stream = application.stream_manager.get_display_stream().stream;
	std::vector<Mpeg::AudioStream>& audio_streams = stream.audio_streams;
	guint count = 0;

	g_debug("Audio streams: %zu", audio_streams.size());
	for (std::vector<Mpeg::AudioStream>::iterator i = audio_streams.begin(); i != audio_streams.end(); i++)
	{
		Mpeg::AudioStream audio_stream = *i;
		Glib::ustring text = Glib::ustring::compose("%1: %2", count, audio_stream.language);
		if (audio_stream.type == STREAM_TYPE_AUDIO_AC3)
		{
			text += " (AC3)";
		}
		else if (audio_stream.type == STREAM_TYPE_AUDIO_MPEG4)
		{
			text += " (MPEG4)";
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

		if (!preferred_language.empty() && (preferred_language == audio_stream.language))
		{
			menu_item->set_active(true);
		}

		count++;
	}

	std::vector<Mpeg::SubtitleStream>& subtitle_streams = stream.subtitle_streams;
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
		
		if (!preferred_language.empty() && (preferred_language == subtitle_stream.language))
		{
			menu_item->set_active(true);
		}

		count++;
	}
}

void MainWindow::start_engine()
{
	if (property_visible())
	{
		play(get_application().stream_manager.get_display_stream().filename);
	}
}

void MainWindow::stop_engine()
{
	g_debug("Stopping engine");

	if (engine != NULL)
	{
		inhibit_screensaver(false);
		engine->stop();
		delete engine;
		engine = NULL;
	}

	g_debug("Engine stopped");
}

void MainWindow::restart_engine()
{
	stop_engine();
	start_engine();
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

void MainWindow::on_channels()
{
	show_channels_dialog();
}

void MainWindow::on_scheduled_recordings()
{
	show_scheduled_recordings_dialog();
	get_application().update();
}

void MainWindow::on_epg_event_search()
{
	show_epg_event_search_dialog();
	get_application().update();
}

void MainWindow::on_preferences()
{
	show_preferences_dialog();
}

void MainWindow::on_about()
{
	FullscreenBugWorkaround fullscreen_bug_workaround;

	Gtk::Dialog* about_dialog = NULL;
	builder->get_widget("dialog_about", about_dialog);
	about_dialog->run();
	about_dialog->hide();
}

void MainWindow::on_mute()
{
	set_mute_state(toggle_action_mute->get_active());
}

void MainWindow::set_mute_state(gboolean state)
{
	mute_state = state;
	if (engine != NULL)
	{
		engine->set_mute_state(mute_state);
	}
}

void MainWindow::on_fullscreen()
{
	if (toggle_action_fullscreen->get_active())
	{
		if (get_application().get_boolean_configuration_value("fullscreen_bug_workaround"))
		{
			maximise_forced = true;
			get_window()->maximize();
		}	

		fullscreen();
	}
	else
	{
		unfullscreen();

		if (maximise_forced)
		{
			get_window()->unmaximize();
			maximise_forced = false;
		}
	}
}

void MainWindow::show_error_dialog(const Glib::ustring& message)
{
	Gtk::MessageDialog dialog(*this, message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
	dialog.set_title(PACKAGE_NAME " - Error");
	dialog.run();
}

void MainWindow::on_exception()
{
	try
	{
		throw;
	}
	catch (const Exception& exception)
	{
		show_error_dialog(exception.what());
	}
	catch (const Glib::Error& exception)
	{
		show_error_dialog(exception.what());
	}
	catch (...)
	{
		show_error_dialog("Unhandled exception");
	}
}

void MainWindow::inhibit_screensaver(gboolean activate)
{
	DBusMessage*	message = NULL;
	DBusMessageIter iter;
	static int		cookie = 0;

	if (dbus_connection == NULL)
	{
		return;
	}
		
	if (activate)
	{
		DBusMessage*	reply = NULL;
		
		message = dbus_message_new_method_call(GS_SERVICE, GS_PATH, GS_INTERFACE, "Inhibit");
		if (message == NULL)
		{
			g_warning ("Couldn't allocate the dbus message");
			return;
		}

		const char* application = PACKAGE_NAME;
		const char* reason = "Playing video";

		dbus_message_iter_init_append (message, &iter);
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &application);
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &reason);

		reply = dbus_connection_send_with_reply_and_block (dbus_connection, message, -1, &dbus_error);
		if (dbus_error_is_set (&dbus_error))
		{
			g_warning ("%s raised:\n %s\n", dbus_error.name, dbus_error.message);
			reply = NULL;
		}

		if (reply != NULL)
		{
			dbus_message_iter_init (reply, &iter);
			dbus_message_iter_get_basic (&iter, &cookie);

			dbus_message_unref (message);
			dbus_message_unref (reply);

			g_debug("Got Cookie: %d", cookie);
			g_debug("Screensaver inhibited");
		}
	}
	else
	{
		if (cookie != 0)
		{
			message = dbus_message_new_method_call(GS_SERVICE, GS_PATH, GS_INTERFACE, "UnInhibit");
			if (message == NULL)
			{
				g_warning ("Couldn't allocate the dbus message");
				return;
			}
			
			g_debug("Using Cookie: %d", cookie);
			dbus_message_iter_init_append (message, &iter);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &cookie);
			dbus_connection_send (dbus_connection, message, NULL);
			cookie = 0;

			g_debug("Screensaver uninhibited");
		}
	}
}
