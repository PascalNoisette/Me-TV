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

#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <libgnomeuimm.h>
#include <libglademm.h>
#include "meters_dialog.h"
#include "channels_dialog.h"
#include "preferences_dialog.h"
#include "ffmpeg_renderer.h"
#include "device_manager.h"

class MainWindow : public Gtk::Window
{
private:
	const Glib::RefPtr<Gnome::Glade::Xml>& glade;
	Gtk::DrawingArea* drawing_area_video;
	FFMpegRenderer renderer;
public:
	MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) : Gtk::Window(cobject), glade(glade)
	{
		drawing_area_video = (Gtk::DrawingArea*)glade->get_widget("drawing_area_video");
		drawing_area_video->modify_bg(Gtk::STATE_NORMAL, Gdk::Color("black"));
		
		glade->connect_clicked("menu_item_open",		sigc::mem_fun(*this, &MainWindow::on_menu_item_open_clicked));
		glade->connect_clicked("menu_item_close",		sigc::mem_fun(*this, &MainWindow::on_menu_item_close_clicked));
		glade->connect_clicked("menu_item_quit",		sigc::mem_fun(*this, &MainWindow::on_menu_item_quit_clicked));
		glade->connect_clicked("menu_item_meters",		sigc::mem_fun(*this, &MainWindow::on_menu_item_meters_clicked));
		glade->connect_clicked("menu_item_channels",	sigc::mem_fun(*this, &MainWindow::on_menu_item_channels_clicked));
		glade->connect_clicked("menu_item_preferences",	sigc::mem_fun(*this, &MainWindow::on_menu_item_preferences_clicked));
		glade->connect_clicked("menu_item_about",		sigc::mem_fun(*this, &MainWindow::on_menu_item_about_clicked));

		Gtk::AboutDialog* dialog_about = (Gtk::AboutDialog*)glade->get_widget("dialog_about");
		dialog_about->set_version(VERSION);
	}
	
	void on_menu_item_open_clicked()
	{
		Gtk::FileChooserDialog dialog(*this, "Open media file ...");
		dialog.add_button(Gtk::Stock::CANCEL, -1);
		dialog.add_button(Gtk::Stock::OPEN, 0);
		dialog.set_default_response(0);
		gint response = dialog.run();
		dialog.hide();
		
		if (response == 0)
		{
			renderer.close();
			Glib::ustring filename = dialog.get_filename();
			g_debug("Playing '%s'", filename.c_str());
			renderer.set_drawing_area(drawing_area_video);
			renderer.open(filename);
		}
	}

	void on_menu_item_close_clicked()
	{
		renderer.close();
	}
		
	void on_menu_item_quit_clicked()
	{
		Gnome::Main::quit();
	}
		
	void on_menu_item_meters_clicked()
	{
		MetersDialog* meters_dialog = NULL;
		glade->get_widget_derived("dialog_meters", meters_dialog);
		Dvb::DeviceManager& device_manager = Application::get_current().get_device_manager();
		const std::list<Dvb::Frontend*> frontends = device_manager.get_frontends();
		if (frontends.size() == 0)
		{
			Gtk::MessageDialog dialog(*this, _("No tuners available"), Gtk::MESSAGE_ERROR);
			dialog.run();
		}
		else
		{
			Dvb::Frontend* frontend = *(frontends.begin());
			meters_dialog->start(*frontend);
			meters_dialog->run();
			meters_dialog->hide();
		}
	}

	void on_menu_item_channels_clicked()
	{
		ChannelsDialog* channels_dialog = NULL;
		glade->get_widget_derived("dialog_channels", channels_dialog);
		channels_dialog->run();
		channels_dialog->hide();
	}

	void on_menu_item_preferences_clicked()
	{
		PreferencesDialog* preferences_dialog = NULL;
		glade->get_widget_derived("dialog_preferences", preferences_dialog);
		preferences_dialog->run();
		preferences_dialog->hide();
	}

	void on_menu_item_about_clicked()
	{
		Gtk::Dialog* about_dialog = NULL;
		glade->get_widget("dialog_about", about_dialog);
		about_dialog->run();
		about_dialog->hide();
	}
};

#endif
