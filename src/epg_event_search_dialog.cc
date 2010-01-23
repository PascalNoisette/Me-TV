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

#include "epg_event_search_dialog.h"
#include "application.h"

EpgEventSearchDialog& EpgEventSearchDialog::create(Glib::RefPtr<Gtk::Builder> builder)
{
	EpgEventSearchDialog* epg_event_search_dialog = NULL;
	builder->get_widget_derived("dialog_epg_event_search", epg_event_search_dialog);
	return *epg_event_search_dialog;
}

EpgEventSearchDialog::EpgEventSearchDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::Dialog(cobject), builder(builder)
{
}
