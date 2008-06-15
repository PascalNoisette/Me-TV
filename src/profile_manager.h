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

#ifndef __PROFILE_MANAGER_H__
#define __PROFILE_MANAGER_H__

#include <glibmm.h>
#include <gconfmm.h>
#include "me-tv.h"
#include "channel_manager.h"

class Profile
{
public:
	Glib::ustring	name;
	ChannelList		channels;
};

typedef std::list<Profile> ProfileList;

class ProfileManager
{
protected:
	Glib::RefPtr<Gnome::Conf::Client> client;
	ProfileList profiles;
	Profile* current_profile;

	Profile* find_profile(const Glib::ustring& profile_name);
public:
	ProfileManager();
	~ProfileManager();
		
	Profile& get_current_profile();
	Profile& get_profile(const Glib::ustring& profile_name);

	void load();
	void save();
};

#endif
