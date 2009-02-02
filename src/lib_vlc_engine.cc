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

#include "lib_vlc_engine.h"

#ifdef ENABLE_LIBVLC_ENGINE

#include "application.h"
#include "exception.h"
#include <vlc/vlc.h>

LibVlcEngine::LibVlcEngine()
{
	Application& application = get_application();

	instance = NULL;
	media_player = NULL;

	Glib::ustring config_file = Glib::build_filename(application.get_application_dir(), "/vlcrc");

	const char * const argv[] = { 
		"-I", "dummy", 
		"--config", config_file.c_str(), 
		"--no-ignore-config", 
		"--save-config", 
		"--no-osd" };

	libvlc_exception_init (&exception);
	check_exception();

	instance = libvlc_new (sizeof(argv) / sizeof(argv[0]), argv, &exception);
	check_exception();

	volume = libvlc_audio_get_volume(instance, &exception);
	check_exception();
}

LibVlcEngine::~LibVlcEngine()
{
	if (media_player != NULL)
	{
		stop();
		libvlc_media_player_release(media_player);
		media_player = NULL;
	}
	if (instance != NULL)
	{
		libvlc_release(instance);
		instance = NULL;
	}
}

void LibVlcEngine::check_exception()
{
	if (libvlc_exception_raised(&exception))
	{
		throw Exception(libvlc_exception_get_message(&exception));
		libvlc_exception_clear(&exception);
	}
}

void LibVlcEngine::play(const Glib::ustring& mrl)
{
	Application& application = get_application();

	if (media_player == NULL)
	{
		media_player = libvlc_media_player_new (instance, &exception);
		check_exception();

		libvlc_media_player_set_drawable(media_player, get_window_id(), &exception);
		check_exception();
	}

	set_volume (volume);

	libvlc_media_t* media = libvlc_media_new(instance, mrl.c_str(), &exception);
	check_exception();

	StringList options;
	options.push_back(Glib::ustring::compose(":vout=%1", application.get_string_configuration_value("vlc.vout")));
	options.push_back(Glib::ustring::compose(":aout=%1", application.get_string_configuration_value("vlc.aout")));
	options.push_back(":video-filter=deinterlace");
	options.push_back(":deinterlace-mode=x"); // discard,blend,mean,bob,linear,x

	for (StringList::iterator iterator = options.begin(); iterator != options.end(); iterator++)
	{
		Glib::ustring process_option = *iterator;
		if (!process_option.empty())
		{
			libvlc_media_add_option(media, process_option.c_str(), &exception);
			check_exception();
		}
	}

	libvlc_media_player_set_media (media_player, media, &exception);
	check_exception();

	libvlc_media_release(media);

	libvlc_media_player_play(media_player, &exception);
	check_exception();
}

void LibVlcEngine::stop()
{
	if(media_player != NULL)
	{
		libvlc_media_player_stop(media_player, &exception);
	}
	
	check_exception();
}

gboolean LibVlcEngine::is_running()
{
	return media_player != NULL;
}

void LibVlcEngine::set_mute_state(gboolean state)
{
	libvlc_audio_set_mute(instance, state, &exception);
	check_exception();
}

void LibVlcEngine::set_volume(gint newlevel)
{
	if (newlevel != volume)
	{
		libvlc_audio_set_volume(instance, newlevel, &exception);
		check_exception();
		volume = libvlc_audio_get_volume(instance, &exception);
		check_exception();
	}
}

#endif
