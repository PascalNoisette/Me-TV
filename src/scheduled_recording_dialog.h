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

#ifndef __SCHEDULED_RECORDING_DIALOG_H__
#define __SCHEDULED_RECORDING_DIALOG_H__

#include <libglademm.h>
#include <libgnomeuimm.h>
#include "epg_event.h"

class ScheduledRecordingDialog : public Gtk::Dialog
{
private:
	const Glib::RefPtr<Gnome::Glade::Xml> glade;
public:
	ScheduledRecordingDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade);

	static ScheduledRecordingDialog* create(Glib::RefPtr<Gnome::Glade::Xml> glade);

	void populate_controls();
	guint run(EpgEvent& epg_event);
	guint run();
};

#endif
