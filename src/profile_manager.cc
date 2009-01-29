/*
 * Copyright (C) 2009 Michael Lamothe
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

#include "profile_manager.h"
#include "application.h"

void ProfileManager::unset_directory(const Glib::ustring& path)
{
	g_debug("Removing %s", path.c_str());
	
	Glib::RefPtr<Gnome::Conf::Client> client = Gnome::Conf::Client::get_default_client();
	StringList directories = client->all_dirs(path);
	StringList::iterator directory_iterator = directories.begin();
	while (directory_iterator != directories.end())
	{
		unset_directory(*directory_iterator);
		directory_iterator++;
	}
	
	std::list<Gnome::Conf::Entry> entries = client->all_entries(path);
	std::list<Gnome::Conf::Entry>::const_iterator entry_iterator = entries.begin();
	while (entry_iterator != entries.end())
	{
		const Glib::ustring& key = (*entry_iterator).get_key();
		client->unset(key);
		entry_iterator++;
	}
	client->clear_cache();
}

ProfileManager::ProfileManager()
{
	current_profile = NULL;
	unset_directory("/apps/me-tv/profiles");
}

ProfileManager::~ProfileManager()
{
}

void ProfileManager::load()
{
	Data data;
	
	profiles = data.get_all_profiles();

	if (profiles.size() == 0)
	{
		current_profile = new Profile();
		current_profile->name = _("Default");
		profiles.push_back(*current_profile);

		save();
		profiles = data.get_all_profiles();
	}
	
	current_profile = find_profile(get_application().get_string_configuration_value("profile"));
	if (current_profile == NULL)
	{
		current_profile = &(*profiles.begin());
	}
}

Profile* ProfileManager::find_profile(const Glib::ustring& profile_name)
{
	Profile* result = NULL;
	ProfileList::iterator iterator = profiles.begin();	
	while (iterator != profiles.end() && result == NULL)
	{
		Profile& profile = *iterator;
		if (profile_name == profile.name)
		{
			result = &profile;
		}
		iterator++;
	}	
	return result;
}

Profile& ProfileManager::get_profile(const Glib::ustring& profile_name)
{
	Profile* profile = find_profile(profile_name);
	if (profile == NULL)
	{
		throw Exception(Glib::ustring::compose(_("Failed to find profile '%1'"), profile_name));
	}
	return *profile;
}

void ProfileManager::save()
{
	for (ProfileList::iterator profile_iterator = profiles.begin(); profile_iterator != profiles.end(); profile_iterator++)
	{
		(*profile_iterator).save();
	}
}

Profile& ProfileManager::get_current_profile()
{
	if (current_profile == NULL)
	{
		throw Exception(_("There is no current profile"));
	}
	return *current_profile;
}
