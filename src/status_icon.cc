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

#include "status_icon.h"
#include "application.h"
#include "channel.h"
#include "me-tv.h"

StatusIcon::StatusIcon()
{
	g_debug("StatusIcon constructor started");
	
	status_icon = Gtk::StatusIcon::create("me-tv");
	status_icon->signal_activate().connect(sigc::mem_fun(*this, &StatusIcon::on_activate));
	status_icon->signal_popup_menu().connect(sigc::mem_fun(*this, &StatusIcon::on_popup_menu));

	Gtk::MenuItem* menu_item = NULL;
	Gtk::ImageMenuItem* image_menu_item = NULL;
	
	menu_status = new Gtk::Menu();

	image_menu_item = new Gtk::ImageMenuItem("Me TV");
	menu_status->append(*image_menu_item);
	image_menu_item->signal_activate().connect(
		sigc::mem_fun(*this, &StatusIcon::on_menu_item_status_me_tv_clicked));

	menu_item = new Gtk::ImageMenuItem(Gtk::Stock::QUIT);
	menu_status->append(*menu_item);
	menu_item->signal_activate().connect(
		sigc::mem_fun(*this, &StatusIcon::on_menu_item_status_quit_clicked));

	menu_status->show_all();

	g_debug("StatusIcon constructed");
}

void StatusIcon::on_popup_menu(guint button, guint32 activate_time)
{
	menu_status->popup(button, activate_time);
}

void StatusIcon::on_menu_item_status_quit_clicked()
{
	get_application().get_main_window().hide();
	Gnome::Main::quit();
}

void StatusIcon::on_menu_item_status_me_tv_clicked()
{
	get_application().get_main_window().toggle_visibility();
}

void StatusIcon::on_activate()
{
	get_application().get_main_window().toggle_visibility();
}

void StatusIcon::update()
{
	Application& application = get_application();
	Glib::ustring title;

	status_icon->set_visible(application.get_boolean_configuration_value("display_status_icon"));
	
	std::list<StreamManager::ChannelStream> streams = application.stream_manager.get_streams();
	if (streams.size() == 0)
	{
		title = _("Unknown program");
	}
	else
	{
		for (std::list<StreamManager::ChannelStream>::iterator i = streams.begin(); i != streams.end(); i++)
		{
			if (title.size() > 0)
			{
				title += "\n";
			}
	
			StreamManager::ChannelStream& stream = *i;
			switch (stream.type)
			{
			case StreamManager::CHANNEL_STREAM_TYPE_DISPLAY: title += "Watching: "; break;
			case StreamManager::CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING: title += "Recording (Scheduled): "; break;
			case StreamManager::CHANNEL_STREAM_TYPE_RECORDING: title += "Recording: "; break;
			default: break;
			}

			title += stream.channel.get_text();
		}	
	}
	
	status_icon->set_tooltip(title);

	if (get_application().stream_manager.is_recording())
	{
		status_icon->set("me-tv-recording");
	}
	else
	{
		status_icon->set("me-tv");
	}
}
