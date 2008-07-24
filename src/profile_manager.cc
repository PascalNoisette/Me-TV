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

#include "profile_manager.h"
#include "me-tv.h"

ProfileManager::ProfileManager() : client(Gnome::Conf::Client::get_default_client())
{
	current_profile = NULL;
	//unset_directory("/apps/me-tv");
	load();
}

ProfileManager::~ProfileManager()
{
	save();
}

void ProfileManager::unset_directory(const Glib::ustring& path)
{
	g_debug("Removing %s", path.c_str());
	
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

void ProfileManager::load()
{	
	Glib::ustring profiles_path = GCONF_PATH"/profiles";
	
	StringList profile_paths = client->all_dirs(profiles_path);
	StringList::iterator profile_iterator = profile_paths.begin();
	while (profile_iterator != profile_paths.end())
	{
		Glib::ustring profile_path = *profile_iterator;

		Profile profile;
		profile.name = client->get_string(profile_path + "/name");

		if (profile.name.empty())
		{
			g_message("Profile at '%s' ignored because the profile name is empty", profile_path.c_str());
		}
		else
		{
			g_debug("Loading '%s' profile", profile.name.c_str());

			Glib::ustring channels_path = profile_path + "/channels";
			StringList channels = client->all_dirs(channels_path);
			StringList::iterator channel_iterator = channels.begin();
			while (channel_iterator != channels.end())
			{
				Glib::ustring channel_path = *channel_iterator;

				Channel channel;
				channel.index			= client->get_int(channel_path + "/index");
				channel.name			= client->get_string(channel_path + "/name");
				channel.pre_command		= client->get_string(channel_path + "/pre_command");
				channel.post_command	= client->get_string(channel_path + "/post_command");
				channel.mrl				= client->get_string(channel_path + "/mrl");
				channel.service_id		= client->get_int(channel_path + "/service_id");
				channel.flags			= client->get_int(channel_path + "/flags");
				
				channel.frontend_parameters.frequency						= client->get_int(channel_path + "/frequency");
				channel.frontend_parameters.u.ofdm.bandwidth				= (fe_bandwidth)			Dvb::Frontend::convert_string_to_value(Dvb::Frontend::get_bandwidth_table(),	client->get_string(channel_path + "/ofdm.bandwidth"));
				channel.frontend_parameters.u.ofdm.code_rate_HP				= (fe_code_rate)			Dvb::Frontend::convert_string_to_value(Dvb::Frontend::get_fec_table(),			client->get_string(channel_path + "/ofdm.code_rate_HP"));
				channel.frontend_parameters.u.ofdm.code_rate_LP				= (fe_code_rate)			Dvb::Frontend::convert_string_to_value(Dvb::Frontend::get_fec_table(),			client->get_string(channel_path + "/ofdm.code_rate_LP"));
				channel.frontend_parameters.u.ofdm.constellation			= (fe_modulation)			Dvb::Frontend::convert_string_to_value(Dvb::Frontend::get_qam_table(),			client->get_string(channel_path + "/ofdm.constellation"));
				channel.frontend_parameters.u.ofdm.transmission_mode		= (fe_transmit_mode)		Dvb::Frontend::convert_string_to_value(Dvb::Frontend::get_modulation_table(),	client->get_string(channel_path + "/ofdm.transmission_mode"));
				channel.frontend_parameters.u.ofdm.guard_interval			= (fe_guard_interval)		Dvb::Frontend::convert_string_to_value(Dvb::Frontend::get_guard_table(),		client->get_string(channel_path + "/ofdm.guard_interval"));
				channel.frontend_parameters.u.ofdm.hierarchy_information	= (fe_hierarchy)			Dvb::Frontend::convert_string_to_value(Dvb::Frontend::get_hierarchy_table(),	client->get_string(channel_path + "/ofdm.hierarchy_information"));
				channel.frontend_parameters.inversion						= (fe_spectral_inversion)	Dvb::Frontend::convert_string_to_value(Dvb::Frontend::get_inversion_table(),	client->get_string(channel_path + "/inversion"));

				g_debug("Channel '%s' read", channel.name.c_str());
				
				profile.channels.push_back(channel);
				channel_iterator++;
			}
			
			profile.channels.sort(channel_sort_by_index);
	
			profiles.push_back(profile);
		}
		
		profile_iterator++;
	}
	
	Glib::ustring current_profile_name = client->get_string(GCONF_PATH"/current_profile");
	if (current_profile_name.empty())
	{
		ProfileList::iterator iterator = profiles.begin();	
		if (iterator != profiles.end())
		{
			// Select first profile as default
			current_profile = &(*iterator);
			g_debug("Selected '%s' as the current profile", current_profile->name.c_str());
		}
		else
		{
			// Create "Default" profile
			g_debug("Creating default profile");
			Profile profile;
			profile.name = "Default";
			profiles.push_back(profile);
			current_profile_name = profile.name;
			current_profile = &(get_profile(profile.name));
		}
	}
	else
	{
		current_profile = &(get_profile(current_profile_name));
		g_debug("Current profile is '%s'", current_profile->name.c_str());
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
	guint profile_count = 0;
	
	client->set(GCONF_PATH"/current_profile", get_current_profile().name);
	
	g_debug("Deleting old profile information");
	Glib::ustring profiles_path = GCONF_PATH"/profiles";
	unset_directory(profiles_path);
	client->suggest_sync();

	g_debug("Saving profile information");
	ProfileList::iterator profile_iterator = profiles.begin();
	while (profile_iterator != profiles.end())
	{
		Profile& profile = *profile_iterator;
		guint channel_count = 0;
	
		Glib::ustring profile_path = Glib::ustring::compose(profiles_path + "/profile_%1", profile_count);

		client->set(profile_path + "/name", profile.name);
		
		ChannelList::iterator channel_iterator = profile.channels.begin();
		while (channel_iterator != profile.channels.end())
		{
			Channel& channel = *channel_iterator;
			
			Glib::ustring channel_path = Glib::ustring::compose(profile_path + "/channels/channel_%1", channel_count);
			
			client->set(channel_path + "/index",		(int)channel_count);
			client->set(channel_path + "/name",			channel.name);
			client->set(channel_path + "/pre_command", 	channel.pre_command);
			client->set(channel_path + "/post_command", channel.post_command);
			client->set(channel_path + "/mrl", 			channel.mrl);
			client->set(channel_path + "/service_id",	(int)channel.service_id);
			client->set(channel_path + "/flags",		(int)channel.flags);

			client->set(channel_path + "/frequency",					(int)channel.frontend_parameters.frequency);
			client->set(channel_path + "/ofdm.bandwidth",				Dvb::Frontend::convert_value_to_string(Dvb::Frontend::get_bandwidth_table(),	channel.frontend_parameters.u.ofdm.bandwidth));
			client->set(channel_path + "/ofdm.code_rate_HP",			Dvb::Frontend::convert_value_to_string(Dvb::Frontend::get_fec_table(),			channel.frontend_parameters.u.ofdm.code_rate_HP));
			client->set(channel_path + "/ofdm.code_rate_LP",			Dvb::Frontend::convert_value_to_string(Dvb::Frontend::get_fec_table(),			channel.frontend_parameters.u.ofdm.code_rate_LP));
			client->set(channel_path + "/ofdm.constellation",			Dvb::Frontend::convert_value_to_string(Dvb::Frontend::get_qam_table(),			channel.frontend_parameters.u.ofdm.constellation));
			client->set(channel_path + "/ofdm.transmission_mode",		Dvb::Frontend::convert_value_to_string(Dvb::Frontend::get_modulation_table(),	channel.frontend_parameters.u.ofdm.transmission_mode));
			client->set(channel_path + "/ofdm.guard_interval",			Dvb::Frontend::convert_value_to_string(Dvb::Frontend::get_guard_table(),		channel.frontend_parameters.u.ofdm.guard_interval));
			client->set(channel_path + "/ofdm.hierarchy_information",	Dvb::Frontend::convert_value_to_string(Dvb::Frontend::get_hierarchy_table(),	channel.frontend_parameters.u.ofdm.hierarchy_information));
			client->set(channel_path + "/inversion",					Dvb::Frontend::convert_value_to_string(Dvb::Frontend::get_inversion_table(),	channel.frontend_parameters.inversion));
			
			channel_count++;
			channel_iterator++;
			
			g_debug("Saved %s", channel.name.c_str());
		}
		
		profile_count++;
		profile_iterator++;
	}
	
	client->suggest_sync();
}

Profile& ProfileManager::get_current_profile()
{
	if (current_profile == NULL)
	{
		throw Exception(_("There is no current profile"));
	}
	return *current_profile;
}
