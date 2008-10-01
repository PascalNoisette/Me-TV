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

#include "main_window.h"
#include "channels_dialog.h"
#include "meters_dialog.h"
#include "preferences_dialog.h"
#include "application.h"
#include "scheduled_recordings_dialog.h"
#include "me-tv.h"
#include "xine_engine.h"
#include "mplayer_engine.h"
#include <config.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <libgnome/libgnome.h>
#include <linux/input.h>
#include <gdk/gdkx.h>

#define POKE_INTERVAL 		30
#define UPDATE_INTERVAL		60

MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade)
	: Gnome::UI::App(cobject), glade(glade)
{
	g_static_rec_mutex_init(mutex.gobj());

	display_mode		= DISPLAY_MODE_EPG;
	last_update_time	= 0;
	last_poke_time		= 0;
	timeout_source		= 0;
	engine				= NULL;
	output_fd			= -1;
	mute_state			= false;
	
	get_signal_error().connect(sigc::mem_fun(*this, &MainWindow::on_error));
	
	app_bar = dynamic_cast<Gnome::UI::AppBar*>(glade->get_widget("app_bar"));
	app_bar->get_progress()->hide();
	drawing_area_video = dynamic_cast<Gtk::DrawingArea*>(glade->get_widget("drawing_area_video"));
	drawing_area_video->set_double_buffered(false);
	drawing_area_video->signal_expose_event().connect(sigc::mem_fun(*this, &MainWindow::on_drawing_area_expose_event));
	
	glade->get_widget_derived("scrolled_window_epg", widget_epg);
	
	if (widget_epg == NULL)
	{
		throw Exception(_("Failed to load EPG widget"));
	}
	
	glade->connect_clicked("menu_item_record",			sigc::mem_fun(*this, &MainWindow::on_menu_item_record_clicked));
	glade->connect_clicked("menu_item_broadcast",		sigc::mem_fun(*this, &MainWindow::on_menu_item_broadcast_clicked));
	glade->connect_clicked("menu_item_quit",			sigc::mem_fun(*this, &MainWindow::on_menu_item_quit_clicked));
	glade->connect_clicked("menu_item_meters",			sigc::mem_fun(*this, &MainWindow::on_menu_item_meters_clicked));
	glade->connect_clicked("menu_item_channels",		sigc::mem_fun(*this, &MainWindow::on_menu_item_channels_clicked));
	glade->connect_clicked("menu_item_preferences",		sigc::mem_fun(*this, &MainWindow::on_menu_item_preferences_clicked));
	glade->connect_clicked("menu_item_fullscreen",		sigc::mem_fun(*this, &MainWindow::on_menu_item_fullscreen_clicked));	
	glade->connect_clicked("menu_item_schedule",		sigc::mem_fun(*this, &MainWindow::show_scheduled_recordings_dialog));	
	glade->connect_clicked("menu_item_mute",			sigc::mem_fun(*this, &MainWindow::on_menu_item_mute_clicked));	
//	glade->connect_clicked("menu_item_help_contents",	sigc::mem_fun(*this, &MainWindow::on_menu_item_help_contents_clicked));	
	glade->connect_clicked("menu_item_about",			sigc::mem_fun(*this, &MainWindow::on_menu_item_about_clicked));	

	glade->connect_clicked("tool_button_record",		sigc::mem_fun(*this, &MainWindow::on_tool_button_record_clicked));	
	glade->connect_clicked("tool_button_mute",			sigc::mem_fun(*this, &MainWindow::on_tool_button_mute_clicked));	
	glade->connect_clicked("tool_button_schedule",		sigc::mem_fun(*this, &MainWindow::show_scheduled_recordings_dialog));	
	glade->connect_clicked("tool_button_broadcast",		sigc::mem_fun(*this, &MainWindow::on_tool_button_broadcast_clicked));	
	
	signal_motion_notify_event().connect(sigc::mem_fun(*this, &MainWindow::on_motion_notify_event));

	Gtk::EventBox* event_box_video = dynamic_cast<Gtk::EventBox*>(glade->get_widget("event_box_video"));
	event_box_video->signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_event_box_video_button_pressed));

	event_box_video->modify_fg(		Gtk::STATE_NORMAL, Gdk::Color("black"));
	drawing_area_video->modify_fg(	Gtk::STATE_NORMAL, Gdk::Color("black"));
	event_box_video->modify_bg(		Gtk::STATE_NORMAL, Gdk::Color("black"));
	drawing_area_video->modify_bg(	Gtk::STATE_NORMAL, Gdk::Color("black"));

	Gtk::AboutDialog* dialog_about = (Gtk::AboutDialog*)glade->get_widget("dialog_about");
	dialog_about->set_version(VERSION);

	is_cursor_visible = true;
	gchar     bits[] = {0};
	GdkColor  color = {0, 0, 0, 0};
	GdkPixmap* pixmap = gdk_bitmap_create_from_data(NULL, bits, 1, 1);
	hidden_cursor = gdk_cursor_new_from_pixmap(pixmap, pixmap, &color, &color, 0, 0);

	last_motion_time = time(NULL);
	timeout_source = gdk_threads_add_timeout(1000, &MainWindow::on_timeout, this);
	
	load_devices();
	set_display_mode(display_mode);
	update();

	Application& application = get_application();
	set_keep_above(application.get_boolean_configuration_value("keep_above"));

	Gtk::MenuItem* menu_item_audio_streams = dynamic_cast<Gtk::MenuItem*>(glade->get_widget("menu_item_audio_streams"));
	menu_item_audio_streams->set_submenu(audio_streams_menu);
	menu_item_audio_streams->hide();
}

MainWindow::~MainWindow()
{
	if (timeout_source != 0)
	{
		g_source_remove(timeout_source);
	}
}

MainWindow* MainWindow::create(Glib::RefPtr<Gnome::Glade::Xml> glade)
{
	MainWindow* main_window = NULL;
	glade->get_widget_derived("application_window", main_window);
	return main_window;
}

void MainWindow::load_devices()
{
	Gtk::MenuItem* menu_item_devices = dynamic_cast<Gtk::MenuItem*>(glade->get_widget("menu_item_devices"));
	Gtk::Menu* menu = menu_item_devices->get_submenu();
	if (menu == NULL)
	{
		menu = new Gtk::Menu();
		menu_item_devices->set_submenu(*menu);
	}
	
	const FrontendList& frontends = get_application().get_device_manager().get_frontends();
	if (frontends.size() == 0)
	{
		Gtk::MenuItem* menu_item = new Gtk::MenuItem(_("No DVB Devices"));
		menu->append(*menu_item);
	}
	else
	{
		for (FrontendList::const_iterator iterator = frontends.begin(); iterator != frontends.end(); iterator++)
		{
			Dvb::Frontend* frontend = *iterator;
			Glib::ustring text = frontend->get_frontend_info().name;
			Gtk::RadioMenuItem* menu_item = new Gtk::RadioMenuItem(radio_button_group_devices, text, false);
			menu->append(*menu_item);
		}
	}

	menu_item_devices->set_submenu(*menu);
	menu_item_devices->show_all();
}

void MainWindow::on_error(const Glib::ustring& message)
{
	Gtk::MessageDialog dialog(*this, message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	dialog.run();
}

void MainWindow::on_menu_item_record_clicked()
{
	TRY
	gboolean record = dynamic_cast<Gtk::CheckMenuItem*>(glade->get_widget("menu_item_record"))->get_active();
	get_application().set_record_state(record);
	CATCH
}

void MainWindow::on_menu_item_broadcast_clicked()
{
	TRY
	gboolean broadcast = dynamic_cast<Gtk::CheckMenuItem*>(glade->get_widget("menu_item_broadcast"))->get_active();
	get_application().set_broadcast_state(broadcast);
	CATCH
}

void MainWindow::on_menu_item_quit_clicked()
{
	TRY
	Gnome::Main::quit();
	CATCH
}
	
void MainWindow::on_menu_item_meters_clicked()
{
	TRY
	
	// Check that there is a device
	get_application().get_device_manager().get_frontend();
	
	meters_dialog = MetersDialog::create(glade);
	meters_dialog->stop();
	meters_dialog->start();
	meters_dialog->show();
	CATCH
}

void MainWindow::on_menu_item_channels_clicked()
{
	TRY
	show_channels_dialog();
	CATCH
}

void MainWindow::show_channels_dialog()
{
	ChannelsDialog& channels_dialog = ChannelsDialog::create(glade);	
	gint dialog_result = channels_dialog.run();
	channels_dialog.hide();
	if (dialog_result == Gtk::RESPONSE_OK)
	{
		Profile& current_profile = get_application().get_profile_manager().get_current_profile();
		ChannelList channels = channels_dialog.get_channels();
		current_profile.set_channels(channels);
		current_profile.save();
	}
	update();

	Glib::RecMutex::Lock lock(mutex);
	if (engine == NULL)
	{
		start_engine();
	}
}

void MainWindow::on_menu_item_preferences_clicked()
{
	TRY
	PreferencesDialog* preferences_dialog = PreferencesDialog::create(glade);
	preferences_dialog->run();
	preferences_dialog->hide();
	update();
	CATCH
}

void MainWindow::on_menu_item_fullscreen_clicked()
{
	TRY
	toggle_fullscreen();
	CATCH
}

void MainWindow::on_menu_item_mute_clicked()
{
	TRY
	gboolean mute = dynamic_cast<Gtk::CheckMenuItem*>(glade->get_widget("menu_item_mute"))->get_active();
	set_mute_state(mute);
	CATCH
}

void MainWindow::toggle_fullscreen()
{
	if (is_fullscreen())
	{
		unfullscreen();
	}
	else
	{
		fullscreen();
	}
}

void MainWindow::on_menu_item_help_contents_clicked()
{
	TRY
	GError* error = NULL;
	if (!gnome_help_display("me-tv", NULL, &error))
	{
		if (error == NULL)
		{
			throw Exception(_("Failed to launch help"));
		}
		else
		{
			throw Exception(Glib::ustring::compose(_("Failed to launch help: %1"), error->message));
		}
	}
	CATCH
}

void MainWindow::on_menu_item_about_clicked()
{
	TRY
	Gtk::Dialog* about_dialog = NULL;
	glade->get_widget("dialog_about", about_dialog);
	about_dialog->run();
	about_dialog->hide();
	CATCH
}

void MainWindow::set_next_display_mode()
{
	if (display_mode == DISPLAY_MODE_VIDEO)
	{
		set_display_mode(DISPLAY_MODE_EPG);
	}
	else if (display_mode == DISPLAY_MODE_EPG)
	{
		set_display_mode(DISPLAY_MODE_CONTROLS);
	}
	else
	{
		set_display_mode(DISPLAY_MODE_VIDEO);
	}
}

bool MainWindow::on_event_box_video_button_pressed(GdkEventButton* event)
{
	TRY
	if (event->button == 1)
	{
		if (event->type == GDK_2BUTTON_PRESS)
		{
			toggle_fullscreen();
		}
	}
	else if (event->button == 3)
	{
		set_next_display_mode();
	}
	CATCH
	
	return true;
}

bool MainWindow::on_motion_notify_event(GdkEventMotion* event)
{
	last_motion_time = time(NULL);
	if (!is_cursor_visible)
	{
		glade->get_widget("event_box_video")->get_window()->set_cursor();
		is_cursor_visible = true;
	}
	return true;
}

void MainWindow::unfullscreen()
{
	Gtk::Window::unfullscreen();
}

void MainWindow::fullscreen()
{
	set_display_mode(DISPLAY_MODE_VIDEO);
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
	TRY
	guint now = time(NULL);
	
	// Hide the mouse
	if (now - last_motion_time > 3 && is_cursor_visible)
	{
		glade->get_widget("event_box_video")->get_window()->set_cursor(Gdk::Cursor(hidden_cursor));
		is_cursor_visible = false;
	}
	
	// Update EPG
	guint application_last_update_time = get_application().get_last_epg_update_time();
	if (((application_last_update_time-2) > last_update_time) || (now - last_update_time > UPDATE_INTERVAL))
	{
		update();
		last_update_time = now;
	}
	
	// Disable screensaver
	if (now - last_poke_time > POKE_INTERVAL)
	{
		Gdk::WindowState state = get_window()->get_state();
		gboolean is_minimised = state & Gdk::WINDOW_STATE_ICONIFIED;
		if (is_visible() && !is_minimised)
		{ 		
			Display* display = GDK_DISPLAY();

			KeyCode keycode = XKeysymToKeycode(display, XK_Shift_L);
			XTestFakeKeyEvent (display, keycode, True, CurrentTime);
			XTestFakeKeyEvent (display, keycode, False, CurrentTime);
			XSync(display, False);
			XFlush(display);
		}
		last_poke_time = now;
	}
	
	CATCH
}

void MainWindow::set_display_mode(DisplayMode mode)
{
	glade->get_widget("menubar")->property_visible()			= (mode == DISPLAY_MODE_EPG) || (mode == DISPLAY_MODE_CONTROLS);
	glade->get_widget("handlebox_toolbar")->property_visible()	= (mode == DISPLAY_MODE_EPG) || (mode == DISPLAY_MODE_CONTROLS);
	glade->get_widget("app_bar")->property_visible()			= (mode == DISPLAY_MODE_EPG) || (mode == DISPLAY_MODE_CONTROLS);
	glade->get_widget("vbox_epg")->property_visible()			= (mode == DISPLAY_MODE_EPG);
		
	display_mode = mode;
}

void MainWindow::show_scheduled_recordings_dialog()
{
	ScheduledRecordingsDialog* scheduled_recordings_dialog = ScheduledRecordingsDialog::create(glade);
	scheduled_recordings_dialog->run();
	scheduled_recordings_dialog->hide();
}

void MainWindow::on_tool_button_record_clicked()
{
	TRY
	gboolean record = dynamic_cast<Gtk::ToggleToolButton*>(glade->get_widget("tool_button_record"))->get_active();
	get_application().set_record_state(record);
	CATCH
}

void MainWindow::on_tool_button_mute_clicked()
{
	TRY
	gboolean mute = dynamic_cast<Gtk::ToggleToolButton*>(glade->get_widget("tool_button_mute"))->get_active();
	set_mute_state(mute);
	CATCH
}

void MainWindow::on_tool_button_broadcast_clicked()
{
	TRY
	gboolean broadcast = dynamic_cast<Gtk::ToggleToolButton*>(glade->get_widget("tool_button_broadcast"))->get_active();
	get_application().set_broadcast_state(broadcast);
	CATCH
}

void MainWindow::update()
{
	Application& application = get_application();
	const Channel* channel = application.get_profile_manager().get_current_profile().get_display_channel();
	Glib::ustring window_title;
	Glib::ustring status_text;
	
	set_state("record", application.is_recording());
	set_state("broadcast", application.is_broadcasting());

	if (channel == NULL)
	{
		window_title = _("Me TV - It's TV for me computer");
	}
	else
	{
		Glib::ustring channel_name = encode_xml(channel->name);
		Glib::ustring name = "<b>" + channel_name + "</b>";
		window_title = "Me TV - " + channel->get_text();
		status_text = channel->get_text();
	}
	
	if (application.is_recording())
	{
		status_text += _(" [Recording]");
		window_title += _(" [Recording]");
	}

	set_title(window_title);
	app_bar->set_status(status_text);

	widget_epg->update();
			
/*	
	Gtk::Menu_Helpers::MenuList& items = audio_streams_menu.items();
	items.erase(items.begin(), items.end());
	
	// Not MT safe!
	StreamThread* stream_thread = application.get_stream_thread();
	if (stream_thread != NULL)
	{
		const Stream& stream = stream_thread->get_stream();
		std::vector<Dvb::SI::AudioStream> audio_streams = stream.audio_streams;
		gint count = 0;
		
		g_debug("Audio streams: %d", audio_streams.size());
		for (std::vector<Dvb::SI::AudioStream>::iterator i = audio_streams.begin(); i != audio_streams.end(); i++)
		{
			Dvb::SI::AudioStream audio_stream = *i;		
			Glib::ustring text = Glib::ustring::compose("%1: %2", ++count, audio_stream.language);
			if (audio_stream.is_ac3)
			{
				text += " (AC3)";
			}
			Gtk::RadioMenuItem* menu_item = new Gtk::RadioMenuItem(audio_streams_menu_group, text);
			menu_item->show_all();
			audio_streams_menu.items().push_back(*menu_item);
		}
	}
*/
}

void MainWindow::set_state(const Glib::ustring& name, gboolean state)
{
	Glib::ustring tool_button_name = "tool_button_" + name;
	Gtk::ToggleToolButton* tool_button = dynamic_cast<Gtk::ToggleToolButton*>(glade->get_widget(tool_button_name));
	if (tool_button->get_active() != state)
	{
		tool_button->set_active(state);
	}

	Glib::ustring menu_item_name = "menu_item_" + name;
	Gtk::CheckMenuItem* menu_item = dynamic_cast<Gtk::CheckMenuItem*>(glade->get_widget(menu_item_name));
	if (menu_item->get_active() != state)
	{
		menu_item->set_active(state);
	}
}

void MainWindow::on_show()
{
	Gtk::Window::on_show();

	TRY
	start_engine();
	if (get_application().get_boolean_configuration_value("keep_above"))
	{
		set_keep_above();
	}
	CATCH
}

void MainWindow::on_hide()
{
	Gtk::Window::on_hide();

	TRY
	stop_engine();
	CATCH
}

void MainWindow::toggle_visibility()
{
	property_visible() = !property_visible();
}

bool MainWindow::on_key_press_event(GdkEventKey* event)
{
	gboolean result = true;
	
	switch(event->keyval)
	{
		case GDK_e:
		case GDK_E:
		case GDK_Mode_switch:
			set_next_display_mode();
			break;
			
		case GDK_f:
		case GDK_F:
			toggle_fullscreen();
			break;

		case GDK_m:
		case GDK_M:
			toggle_mute();
			break;

		case GDK_r:
		case GDK_R:
			get_application().toggle_recording();
			break;
		
		case GDK_Up:
		case GDK_minus:
			get_application().get_profile_manager().get_current_profile().previous_channel();
			break;
			
		case GDK_plus:
		case GDK_Down:
			get_application().get_profile_manager().get_current_profile().next_channel();
			break;

		default:
			result = false;
			break;
	}
	
	return result;
}

bool MainWindow::on_drawing_area_expose_event(GdkEventExpose* event)
{
	TRY
	Glib::RecMutex::Lock lock(mutex);
	if (engine == NULL)
	{
		drawing_area_video->get_window()->draw_rectangle(
			drawing_area_video->get_style()->get_bg_gc(Gtk::STATE_NORMAL),
			true, event->area.x, event->area.y, event->area.width, event->area.height);
	}
	else
	{
		engine->set_size(event->area.width, event->area.height);
		engine->expose();
	}
	CATCH

	return false;
}

class EngineStartThread : public Thread
{
private:
	Engine*					engine;
	const Glib::ustring&	fifo_path;

	void run()
	{
		g_debug("Telling engine to start playing");
		engine->play(fifo_path);
		g_debug("Engine playing");
	}

public:
	EngineStartThread(Engine* engine, const Glib::ustring& fifo_path)
		: Thread("Engine start thread"), engine(engine), fifo_path(fifo_path) {}
};

class StopEngineThread : public Thread
{
private:
	Engine*	engine;
	
	void run()
	{
		g_debug("Stopping engine in thread");
		engine->stop();
		g_debug("Engine stopped in thread");
	}
public:
	StopEngineThread(Engine* engine) : Thread("Stop engine"), engine(engine) {}
};

void MainWindow::start_engine()
{
	Glib::RecMutex::Lock lock(mutex);

	// Acquire stream thread lock
	Application& application = get_application();
	Glib::RecMutex::Lock application_lock(application.get_mutex());
	
	StreamThread* stream_thread = application.get_stream_thread();
	if (property_visible() && stream_thread != NULL)
	{
		g_debug("Starting engine");

		Dvb::Frontend& frontend = application.get_device_manager().get_frontend();
		if (engine != NULL)
		{
			throw Exception(_("Failed to start engine: Engine has already been started"));
		}
		
		Gtk::DrawingArea* drawing_area_video = dynamic_cast<Gtk::DrawingArea*>(glade->get_widget("drawing_area_video"));
		int window_id = GDK_WINDOW_XID(drawing_area_video->get_window()->gobj());

		if (window_id == 0)
		{
			throw Exception(_("Window ID was 0"));
		}
		
		g_debug("Creating engine");
		Glib::ustring engine_type = application.get_string_configuration_value("engine_type");
		if (engine_type == "xine")
		{
			engine = new XineEngine(window_id);
		}
		else if (engine_type == "mplayer")
		{
			engine = new MplayerEngine(window_id);
		}
		else
		{
			throw Exception(_("Unknown engine type"));
		}
		g_debug("%s engine created", engine_type.c_str());
		
		Glib::ustring filename = Glib::ustring::compose("me-tv-%1.fifo", frontend.get_adapter().get_index());
		Glib::ustring fifo_path = Glib::build_filename(Glib::get_home_dir(), ".me-tv");
		fifo_path = Glib::build_filename(fifo_path, filename);
		
		if (!Glib::file_test(fifo_path, Glib::FILE_TEST_EXISTS))
		{
			if (mkfifo(fifo_path.c_str(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0)
			{
				throw Exception(Glib::ustring::compose(_("Failed to create FIFO '%1'"), fifo_path));
			}
		}

		if (engine != NULL)
		{
			gint width, height;
			drawing_area_video->get_window()->get_size(width, height);
			engine->set_size(width, height);
			engine->mute(mute_state);

			EngineStartThread engine_start_thread(engine, fifo_path);
			engine_start_thread.start();
			
			g_debug("Opening '%s'", fifo_path.c_str());
			output_fd = open(fifo_path.c_str(), O_WRONLY, 0);
			
			if (output_fd == -1)
			{
				throw SystemException(_("Failed to create FIFO output"));
			}

			stream_thread->connect_output(output_fd);
			
			g_debug("Output FD created");
		}

		g_debug("Engine started");
	}
}

void MainWindow::stop_engine()
{
	g_debug("Stopping engine");
	Glib::RecMutex::Lock lock(mutex);

	if (engine != NULL)
	{
		StopEngineThread stop_engine_thread(engine);
		stop_engine_thread.start();

		get_application().connect_output(-1);
		if (output_fd != -1)
		{
			close(output_fd);
			output_fd = -1;
		}

		stop_engine_thread.join(true);
		
		delete engine;
		engine = NULL;
	}

	g_debug("Engine stopped");
}

void MainWindow::toggle_mute()
{
	set_mute_state(!mute_state);
}

void MainWindow::set_mute_state(gboolean state)
{
	if (mute_state != state)
	{
		mute_state = state;
		g_message("Setting mute to %s", mute_state ? "true" : "false");	
		set_state("mute", mute_state);

		{
			Glib::RecMutex::Lock lock(mutex);
			if (engine != NULL)
			{
				engine->mute(mute_state);
			}
		}

		update();
	}
}
