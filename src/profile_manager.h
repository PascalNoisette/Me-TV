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

class Profile
{
private:
	Glib::ustring name;
	Glib::ustring path;
	std::list<Glib::ustring> channels;
public:
	Profile(Glib::RefPtr<Gnome::Conf::Client> client, const Glib::ustring& node)
	{
		path = "/apps/me-tv/profiles/" + node;
		name = client->get_string(path + "/name");
		channels = client->get_string_list(path + "/channels");
	}
		
	const Glib::ustring& get_name() const { return name; }
	std::list<Glib::ustring>& get_channels() { return channels; }
};

class ProfileManager
{
protected:
	Glib::RefPtr<Gnome::Conf::Client> client;
public:
	ProfileManager() : client(Gnome::Conf::Client::get_default_client())
	{
	}
		
	Profile get_current()
	{
		Glib::ustring current_profile = client->get_string("/apps/me-tv/current_profile");
		if (current_profile.empty())
		{
			current_profile = "profile_1";
			client->set("/apps/me-tv/current_profile", current_profile);
		}
		if (!client->dir_exists("/apps/me-tv/profiles/" + current_profile))
		{
			client->set("/apps/me-tv/profiles/profile_1/name", Glib::ustring("Default"));
		}
		return Profile(client, current_profile);
	}
		
	void set_current(const Profile& profile)
	{
		client->set("/apps/me-tv/current_profile", profile.get_name());
	}
};

#endif
