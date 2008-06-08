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

#ifndef __CHANNELS_DIALOG_H__
#define __CHANNELS_DIALOG_H__

#include <libgnomeuimm.h>
#include <libglademm.h>
#include "scan_dialog.h"
#include "application.h"

class ChannelsDialog : public Gtk::Dialog
{
private:
	const Glib::RefPtr<Gnome::Glade::Xml>& glade;
public:	
	ChannelsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) :
		Gtk::Dialog(cobject), glade(glade)
	{
		glade->connect_clicked("button_scan", sigc::mem_fun(*this, &ChannelsDialog::on_button_scan_clicked));
		
		ProfileManager& profile_manager = Application::get_current().get_profile_manager();
		Profile profile = profile_manager.get_current();
		
		ComboBoxEntryText* combo_box_entry_text_profile = NULL;
		glade->get_widget_derived("combo_box_entry_profile", combo_box_entry_text_profile);
		combo_box_entry_text_profile->append_text(profile.get_name());
		combo_box_entry_text_profile->set_active_text(profile.get_name());
	}
		
	void on_button_scan_clicked()
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
			scan_dialog->run();
			scan_dialog->hide();
			
			std::list<Dvb::Service> selected_services = scan_dialog->get_selected_services();
		}
	}
};

#endif
