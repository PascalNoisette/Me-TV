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
#include "pipeline_manager.h"
#include "application.h"
#include "scan_dialog.h"
#include <config.h>

MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) : Gtk::Window(cobject), glade(glade)
{
	drawing_area_video = dynamic_cast<Gtk::DrawingArea*>(glade->get_widget("drawing_area_video"));
	drawing_area_video->modify_bg(Gtk::STATE_NORMAL, Gdk::Color("black"));
	
	glade->get_widget_derived("vbox_epg", widget_epg);
	
	glade->connect_clicked("menu_item_open",		sigc::mem_fun(*this, &MainWindow::on_menu_item_open_clicked));
	glade->connect_clicked("menu_item_close",		sigc::mem_fun(*this, &MainWindow::on_menu_item_close_clicked));
	glade->connect_clicked("menu_item_quit",		sigc::mem_fun(*this, &MainWindow::on_menu_item_quit_clicked));
	glade->connect_clicked("menu_item_meters",		sigc::mem_fun(*this, &MainWindow::on_menu_item_meters_clicked));
	glade->connect_clicked("menu_item_channels",	sigc::mem_fun(*this, &MainWindow::on_menu_item_channels_clicked));
	glade->connect_clicked("menu_item_preferences",	sigc::mem_fun(*this, &MainWindow::on_menu_item_preferences_clicked));
	glade->connect_clicked("menu_item_about",		sigc::mem_fun(*this, &MainWindow::on_menu_item_about_clicked));

	Gtk::EventBox* event_box_video = dynamic_cast<Gtk::EventBox*>(glade->get_widget("event_box_video"));
	event_box_video->signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_event_box_video_button_pressed));
	event_box_video->signal_motion_notify_event().connect(sigc::mem_fun(*this, &MainWindow::on_event_box_video_motion_notify_event));
	event_box_video->signal_scroll_event().connect(sigc::mem_fun(*this, &MainWindow::on_event_box_video_scroll_event));

	Gtk::AboutDialog* dialog_about = (Gtk::AboutDialog*)glade->get_widget("dialog_about");
	dialog_about->set_version(VERSION);

	is_cursor_visible = true;
	gchar     bits[] = {0};
	GdkColor  color = {0, 0, 0, 0};
	GdkPixmap* pixmap = gdk_bitmap_create_from_data(NULL, bits, 1, 1);
	hidden_cursor = gdk_cursor_new_from_pixmap(pixmap, pixmap, &color, &color, 0, 0);

	last_motion_time = time(NULL);
	Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &MainWindow::on_timeout), 1);

	glade->get_widget_derived("dialog_meters", meters_dialog);

	show();
	glade->get_widget("hbox_search_bar")->hide();

	widget_epg->update();
	
	get_signal_error().connect(sigc::mem_fun(*this, &MainWindow::on_error));
	
	//drawing_area_video->set_double_buffered(tr);
	set_keep_above();
}

MainWindow::~MainWindow()
{
	stop();
}

void MainWindow::on_error(const Glib::ustring& message)
{
	Gtk::MessageDialog dialog(*this, message);
	dialog.run();
}

Gtk::DrawingArea& MainWindow::get_drawing_area()
{
	if (drawing_area_video == NULL)
	{
		throw Exception(_("The video drawing area has not been created"));
	}
	return *drawing_area_video;
}
	
void MainWindow::on_menu_item_open_clicked()
{
	TRY
	Gtk::FileChooserDialog dialog(*this, "Open media file ...");
	dialog.add_button(Gtk::Stock::CANCEL, -1);
	dialog.add_button(Gtk::Stock::OPEN, 0);
	dialog.set_default_response(0);
	gint response = dialog.run();
	dialog.hide();
	
	if (response == 0)
	{
		Glib::ustring filename = dialog.get_filename();
		g_debug("Playing '%s'", filename.c_str());
		
		stop();
		
		PipelineManager& pipeline_manager = get_application().get_pipeline_manager();
		Pipeline& pipeline = pipeline_manager.create("display", filename, *drawing_area_video);
		pipeline.start();
	}
	CATCH
}

void MainWindow::on_menu_item_close_clicked()
{
	stop();
}
	
void MainWindow::on_menu_item_quit_clicked()
{
	Gnome::Main::quit();
}
	
void MainWindow::on_menu_item_meters_clicked()
{
	Dvb::DeviceManager& device_manager = get_application().get_device_manager();
	const std::list<Dvb::Frontend*> frontends = device_manager.get_frontends();
	if (frontends.size() == 0)
	{
		Gtk::MessageDialog dialog(*this, _("No tuners available"), Gtk::MESSAGE_ERROR);
		dialog.run();
	}
	else
	{
		meters_dialog->stop();
		Dvb::Frontend* frontend = *(frontends.begin());
		meters_dialog->start(*frontend);
		meters_dialog->show();
	}
}

void MainWindow::on_menu_item_channels_clicked()
{
	gboolean done = false;
	
	ChannelManager& channel_manager = get_application().get_channel_manager();
	ChannelsDialog* channels_dialog = NULL;
	glade->get_widget_derived("dialog_channels", channels_dialog);
	channels_dialog->set_channels(channel_manager.get_channels());
	
	while (!done)
	{
		gint result = channels_dialog->run();
		channels_dialog->hide();
		widget_epg->update();
		
		// Pressed Cancel
		if (result == Gtk::RESPONSE_CANCEL || result == Gtk::RESPONSE_DELETE_EVENT)
		{
			done = true;
		}

		// Pressed OK
		else if (result == Gtk::RESPONSE_OK)
		{
			ChannelList channels = channels_dialog->get_channels();
			channel_manager.clear();
			channel_manager.add_channels(channels);
			
			get_application().get_profile_manager().get_current_profile().channels = channels;
			
			widget_epg->update();
			done = true;
		}
		
		// Pressed scan button
		else if (result == 1)
		{
			gsize frontend_count = Application::get_current().get_device_manager().get_frontends().size();
			if (frontend_count == 0)
			{
				Gtk::MessageDialog dialog(*this, "There are no tuners to scan", false, Gtk::MESSAGE_ERROR);
				dialog.run();
			}
			else
			{
				ScanDialog* scan_dialog = NULL;
				glade->get_widget_derived("dialog_scan", scan_dialog);
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
}

void MainWindow::on_menu_item_preferences_clicked()
{
	PreferencesDialog* preferences_dialog = NULL;
	glade->get_widget_derived("dialog_preferences", preferences_dialog);
	preferences_dialog->run();
	preferences_dialog->hide();
	widget_epg->update();
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
	if (event->button == 1)
	{
		if (event->type == GDK_2BUTTON_PRESS)
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
	}
	else if (event->button == 3)
	{
		gboolean show = !glade->get_widget("vbox_epg")->property_visible();
		glade->get_widget("vbox_epg")->property_visible() = show;
		glade->get_widget("menubar")->property_visible() = show;
		glade->get_widget("handlebox_toolbar")->property_visible() = show;		
		glade->get_widget("statusbar")->property_visible() = show || !is_fullscreen();
		
		if (show)
		{
			widget_epg->update();
		}
	}
	
	return true;
}

bool MainWindow::on_event_box_video_motion_notify_event(GdkEventMotion* event)
{
	last_motion_time = time(NULL);
	glade->get_widget("event_box_video")->get_window()->set_cursor();
	is_cursor_visible = true;
	
	return true;
}

bool MainWindow::on_event_box_video_scroll_event(GdkEventScroll* event)
{
	switch(event->direction)
	{
		case GDK_SCROLL_UP:
			{
//				PipelineManager& pipeline_manager = get_application().get_pipeline_manager();
//				Pipeline& pipeline = pipeline_manager.get_pipeline("display");
			}
			break;
		case GDK_SCROLL_DOWN:
			{
//				PipelineManager& pipeline_manager = get_application().get_pipeline_manager();
//				Pipeline& pipeline = pipeline_manager.get_pipeline("display");
//				pipeline.get_source().seek(0);
			}
			break;
	}
	
	return true;
}

void MainWindow::unfullscreen()
{
	glade->get_widget("menubar")->show();
	glade->get_widget("statusbar")->show();
	glade->get_widget("handlebox_toolbar")->show();
	glade->get_widget("vbox_epg")->show();

	Gtk::Window::unfullscreen();
}

void MainWindow::fullscreen()
{
	glade->get_widget("menubar")->hide();
	glade->get_widget("statusbar")->hide();
	glade->get_widget("handlebox_toolbar")->hide();
	glade->get_widget("vbox_epg")->hide();

	Gtk::Window::fullscreen();
}

gboolean MainWindow::is_fullscreen()
{
	return get_window()->get_state() & Gdk::WINDOW_STATE_FULLSCREEN;
}

bool MainWindow::on_timeout()
{
	if (time(NULL) - last_motion_time > 3 && is_cursor_visible)
	{
		GdkLock gdk_lock();
		glade->get_widget("event_box_video")->get_window()->set_cursor(Gdk::Cursor(hidden_cursor));
		is_cursor_visible = false;
	}
	
	return true;
}

void MainWindow::stop()
{
	PipelineManager& pipeline_manager = get_application().get_pipeline_manager();
	Pipeline* pipeline = pipeline_manager.find_pipeline("display");
	if (pipeline != NULL)
	{
		pipeline_manager.remove(pipeline);
	}
}
