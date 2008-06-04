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

class ChannelsDialog : public Gtk::Dialog
{
private:
	const Glib::RefPtr<Gnome::Glade::Xml>& glade;
public:	
	ChannelsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) : Gtk::Dialog(cobject), glade(glade)
	{
		glade->connect_clicked("button_scan", sigc::mem_fun(*this, &ChannelsDialog::on_menu_item_scan_clicked));
	}
		
	void on_menu_item_scan_clicked()
	{
		ScanDialog* scan_dialog = NULL;
		glade->get_widget_derived("dialog_scan", scan_dialog);
		scan_dialog->run();
		scan_dialog->hide();
	}
};

#endif
