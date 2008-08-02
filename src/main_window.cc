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
#include "scan_dialog.h"
#include "scheduled_recordings_dialog.h"
#include "me-tv.h"
#include <config.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <gdk/gdkx.h>

#define POKE_INTERVAL 30

MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade)
	: Gnome::UI::App(cobject), glade(glade)
{
	display_mode = DISPLAY_MODE_EPG;
	last_update_time = 0;
	
	app_bar = dynamic_cast<Gnome::UI::AppBar*>(glade->get_widget("app_bar"));
	app_bar->get_progress()->hide();
	drawing_area_video = dynamic_cast<Gtk::DrawingArea*>(glade->get_widget("drawing_area_video"));
	drawing_area_video->set_double_buffered(false);
//	drawing_area_video->signal_expose_event().connect(sigc::mem_fun(*this, &MainWindow::on_drawing_area_video_expose));
	
	glade->get_widget_derived("scrolled_window_epg", widget_epg);
	
	if (widget_epg == NULL)
	{
		throw Exception("Failed to load EPG widget");
	}
	
	glade->connect_clicked("menu_item_quit",		sigc::mem_fun(*this, &MainWindow::on_menu_item_quit_clicked));
	glade->connect_clicked("menu_item_meters",		sigc::mem_fun(*this, &MainWindow::on_menu_item_meters_clicked));
	glade->connect_clicked("menu_item_channels",	sigc::mem_fun(*this, &MainWindow::on_menu_item_channels_clicked));
	glade->connect_clicked("menu_item_preferences",	sigc::mem_fun(*this, &MainWindow::on_menu_item_preferences_clicked));
	glade->connect_clicked("menu_item_fullscreen",	sigc::mem_fun(*this, &MainWindow::on_menu_item_fullscreen_clicked));	
	glade->connect_clicked("menu_item_scheduled_recordings",	sigc::mem_fun(*this, &MainWindow::show_scheduled_recordings_dialog));	
	glade->connect_clicked("menu_item_about",		sigc::mem_fun(*this, &MainWindow::on_menu_item_about_clicked));	

	glade->connect_clicked("tool_button_record", sigc::mem_fun(*this, &MainWindow::on_tool_button_record_clicked));	
	glade->connect_clicked("tool_button_mute", sigc::mem_fun(*this, &MainWindow::on_tool_button_mute_clicked));	
	glade->connect_clicked("tool_button_scheduled_recordings", sigc::mem_fun(*this, &MainWindow::show_scheduled_recordings_dialog));	
	
	signal_motion_notify_event().connect(sigc::mem_fun(*this, &MainWindow::on_motion_notify_event));

	Gtk::EventBox* event_box_video = dynamic_cast<Gtk::EventBox*>(glade->get_widget("event_box_video"));
	event_box_video->signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_event_box_video_button_pressed));
	event_box_video->signal_scroll_event().connect(sigc::mem_fun(*this, &MainWindow::on_event_box_video_scroll_event));

	event_box_video->modify_fg(Gtk::STATE_NORMAL, Gdk::Color("black"));
	drawing_area_video->modify_fg(Gtk::STATE_NORMAL, Gdk::Color("black"));
	event_box_video->modify_bg(Gtk::STATE_NORMAL, Gdk::Color("black"));
	drawing_area_video->modify_bg(Gtk::STATE_NORMAL, Gdk::Color("black"));

	Gtk::AboutDialog* dialog_about = (Gtk::AboutDialog*)glade->get_widget("dialog_about");
	dialog_about->set_version(VERSION);

	is_cursor_visible = true;
	gchar     bits[] = {0};
	GdkColor  color = {0, 0, 0, 0};
	GdkPixmap* pixmap = gdk_bitmap_create_from_data(NULL, bits, 1, 1);
	hidden_cursor = gdk_cursor_new_from_pixmap(pixmap, pixmap, &color, &color, 0, 0);

	last_motion_time = time(NULL);
	initialise = true;
	Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &MainWindow::on_timeout), 1);
	
	load_devices();
	show();
	set_display_mode(display_mode);
	update();
	
	Profile& current_profile = get_application().get_profile_manager().get_current_profile();
	get_signal_error().connect(sigc::mem_fun(*this, &MainWindow::on_error));
	current_profile.signal_display_channel_changed.connect(
		sigc::mem_fun(*this, &MainWindow::on_display_channel_changed));
	
	if (get_application().get_boolean_configuration_value("keep_above"))
	{
		set_keep_above();
	}
}

MainWindow::~MainWindow()
{
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
		Gtk::MenuItem* menu_item = new Gtk::MenuItem("No DVB Devices");
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
	Gtk::MessageDialog dialog(*this, message);
	dialog.run();
}

void MainWindow::on_display_channel_changed(const Channel& channel)
{
	TRY
	update();
	CATCH;
}

Gtk::DrawingArea& MainWindow::get_drawing_area()
{
	if (drawing_area_video == NULL)
	{
		throw Exception(_("The video drawing area has not been created"));
	}
	return *drawing_area_video;
}

void MainWindow::on_menu_item_quit_clicked()
{
	Gnome::Main::quit();
}
	
void MainWindow::on_menu_item_meters_clicked()
{
	TRY
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
	gboolean done = false;
	
	Profile& profile = get_application().get_profile_manager().get_current_profile();
	ChannelsDialog* channels_dialog = ChannelsDialog::create(glade);
	channels_dialog->set_channels(profile.get_channels());
	
	while (!done)
	{
		gint result = channels_dialog->run();
		channels_dialog->hide();
		update();
		
		// Pressed Cancel
		if (result == Gtk::RESPONSE_CANCEL || result == Gtk::RESPONSE_DELETE_EVENT)
		{
			done = true;
		}

		// Pressed OK
		else if (result == Gtk::RESPONSE_OK)
		{
			ChannelList channels = channels_dialog->get_channels();
			profile.clear();
			profile.add_channels(channels);
			
			// Must save profile/channels to get updated Channels PK IDs
			Data data;
			data.replace_profile(profile);
			
			update();
			done = true;
		}
		
		// Pressed scan button
		else if (result == 1)
		{
			ScanDialog* scan_dialog = ScanDialog::create(glade);
			guint scan_dialog_result = scan_dialog->run();
			scan_dialog->hide();
		
			if (scan_dialog_result == Gtk::RESPONSE_OK)
			{
				std::list<ScannedService> scanned_services = scan_dialog->get_scanned_services();
				channels_dialog->add_scanned_services(scanned_services);
			}
		}
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
	toggle_fullscreen();
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

void MainWindow::on_menu_item_about_clicked()
{
	Gtk::Dialog* about_dialog = NULL;
	glade->get_widget("dialog_about", about_dialog);
	about_dialog->run();
	about_dialog->hide();
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

bool MainWindow::on_event_box_video_scroll_event(GdkEventScroll* event)
{
	switch(event->direction)
	{
	case GDK_SCROLL_UP:
		{
		}
		break;
	case GDK_SCROLL_DOWN:
		{
		}
		break;
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

bool MainWindow::on_timeout()
{
	static time_t last_poke_time = 0;

	TRY
	guint now = time(NULL);
	
	// Async initialisation
	if (initialise)
	{
		initialise = false;
		GdkLock gdk_lock();
		Profile& current_profile = get_application().get_profile_manager().get_current_profile();
		ChannelList& channels = current_profile.get_channels();
		if (channels.size() > 0)
		{
			gint last_channel = get_application().get_int_configuration_value("last_channel");
			if (last_channel == -1)
			{
				last_channel = (*channels.begin()).channel_id;
			}
			current_profile.set_display_channel(last_channel);
		}
		else
		{
			show_channels_dialog();
		}
	}
	
	// Hide the mouse
	if (now - last_motion_time > 3 && is_cursor_visible)
	{
		GdkLock gdk_lock();
		glade->get_widget("event_box_video")->get_window()->set_cursor(Gdk::Cursor(hidden_cursor));
		is_cursor_visible = false;
	}
	
	// Update EPG
	guint application_last_update_time = get_application().get_last_epg_update_time();
	if (application_last_update_time > last_update_time)
	{
		GdkLock gdk_lock();
		update();
		last_update_time = application_last_update_time;
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
	
	THREAD_CATCH
		
	return true;
}

void MainWindow::set_display_mode(DisplayMode mode)
{
	glade->get_widget("menubar")->property_visible()			= (mode == DISPLAY_MODE_EPG) || (mode == DISPLAY_MODE_CONTROLS);
	glade->get_widget("handlebox_toolbar")->property_visible()	= (mode == DISPLAY_MODE_EPG) || (mode == DISPLAY_MODE_CONTROLS);
	glade->get_widget("app_bar")->property_visible()			= (mode == DISPLAY_MODE_EPG) || (mode == DISPLAY_MODE_CONTROLS);
	widget_epg->property_visible()								= (mode == DISPLAY_MODE_EPG);

	Gtk::VBox* vbox_main = dynamic_cast<Gtk::VBox*>(glade->get_widget("vbox_main"));
	Gtk::DrawingArea* drawing_area_video = dynamic_cast<Gtk::DrawingArea*>(glade->get_widget("drawing_area_video"));
	Gtk::EventBox* event_box_video = dynamic_cast<Gtk::EventBox*>(glade->get_widget("event_box_video"));
	
	gtk_box_set_child_packing((GtkBox*)vbox_main->gobj(), (GtkWidget*)event_box_video->gobj(),
		(mode != DISPLAY_MODE_EPG), (mode != DISPLAY_MODE_EPG), 0, GTK_PACK_START);
	event_box_video->set_size_request(-1, 100);
	
	display_mode = mode;
}

bool MainWindow::on_drawing_area_video_expose(GdkEventExpose* event)
{
	Glib::RefPtr<const Gdk::GC> gc = drawing_area_video->get_style()->get_fg_gc(drawing_area_video->get_state());
	drawing_area_video->get_window()->draw_rectangle(gc, true,
		event->area.x, event->area.y, event->area.width, event->area.height);
	return false;
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
	Application& application = get_application();
	Gtk::ToggleToolButton* tool_button_record = dynamic_cast<Gtk::ToggleToolButton*>(glade->get_widget("tool_button_record"));
	if (tool_button_record->get_active())
	{
		const Channel* channel = application.get_profile_manager().get_current_profile().get_display_channel();
		if (channel == NULL)
		{
			throw Exception(_("No current channel"));
		}
		
		Glib::ustring recording_directory = application.get_string_configuration_value("recording_directory");
		Glib::ustring path = Glib::build_filename(recording_directory, channel->get_text() + ".mpeg");
		application.record(path);
	}
	else
	{
		get_application().record("");
	}
	CATCH
}

void MainWindow::on_tool_button_mute_clicked()
{
	TRY
	Gtk::ToggleToolButton* tool_button_mute = dynamic_cast<Gtk::ToggleToolButton*>(glade->get_widget("tool_button_mute"));
	get_application().mute(tool_button_mute->get_active());
	CATCH
}

gboolean MainWindow::get_mute_state()
{
	Gtk::ToggleToolButton* tool_button_mute = dynamic_cast<Gtk::ToggleToolButton*>(glade->get_widget("tool_button_mute"));
	return tool_button_mute->get_active();
}

void MainWindow::update()
{
	const Channel* channel = get_application().get_profile_manager().get_current_profile().get_display_channel();
	Glib::ustring window_title;
	Glib::ustring status_text;
	
	if (channel == NULL)
	{
		window_title = "Me TV - It's TV for me computer";
	}
	else
	{
		Glib::ustring channel_name = encode_xml(channel->name);
		Glib::ustring name = "<b>" + channel_name + "</b>";
		window_title = "Me TV - " + channel->get_text();
		status_text = channel->get_text();
	}
	set_title(window_title);
	app_bar->set_status(status_text);

	widget_epg->update();
}
