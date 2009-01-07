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

#include "application.h"
#include "vlc_engine.h"
#include "exception.h"

#define DEFAULT_VOLUME	130

VlcEngine::VlcEngine(int window_id) : window_id(window_id)
{
	const char * const argv[] = { 
		"-I", "dummy", 
		"--ignore-config", 
		"--no-osd" };

	libvlc_exception_init (&exception);
	check_exception();

	instance = libvlc_new (sizeof(argv) / sizeof(argv[0]), argv, &exception);
	check_exception();

	mute_state = libvlc_audio_get_mute(instance, &exception);
	check_exception();
	volume = libvlc_audio_get_volume(instance, &exception);
	check_exception();
}

VlcEngine::~VlcEngine()
{
#ifdef USE_VLC_0_8_6_API
	libvlc_destroy (instance);
#else
	libvlc_media_player_release(media_player);
	media_player = NULL;
	libvlc_release(instance);
#endif
	instance = NULL;
}

void VlcEngine::check_exception()
{
	if (libvlc_exception_raised(&exception))
	{
		throw Exception(libvlc_exception_get_message(&exception));
	}
}

void VlcEngine::play(const Glib::ustring& mrl)
{
#ifdef USE_VLC_0_8_6_API
	int item = libvlc_playlist_add(instance, mrl.c_str(), NULL, &exception);
	check_exception();

	libvlc_playlist_play(instance, item, 0, NULL, &exception); 
	check_exception();
#else
	Application& application = get_application();

	media_player = libvlc_media_player_new (instance, &exception);
	check_exception();

	libvlc_media_player_set_drawable(media_player, window_id, &exception);
	check_exception();

	libvlc_audio_set_volume (instance, volume, &exception);
	check_exception();

	libvlc_media_t* media = libvlc_media_new(instance, mrl.c_str(), &exception);
	check_exception();

	StringList options;
	options.push_back(":ignore-config=1");
	options.push_back(":osd=0");
	options.push_back(":file-caching=5000");	// ms
	options.push_back(Glib::ustring::compose(":vout=%1", application.get_string_configuration_value("vlc.vout")));
	options.push_back(Glib::ustring::compose(":aout=%1", application.get_string_configuration_value("vlc.aout")));
	options.push_back(":skip-frames=0");
	options.push_back(":drop-late-frames=1");
/*
	options.push_back(":video-filter=deinterlace");
	options.push_back(":vout-filter=deinterlace");
	options.push_back(":deinterlace-mode=bob"); // discard,blend,mean,bob,linear,x
*/
	options.push_back(":postproc-q=6");

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

	mute(false);
	set_volume(DEFAULT_VOLUME);
#endif
}

void VlcEngine::stop()
{
#ifdef USE_VLC_0_8_6_API
	libvlc_playlist_stop(instance, &exception);
#else
	if(media_player != NULL)
	{
		libvlc_media_player_stop(media_player, &exception);
	}
#endif
	
	check_exception();
}

gboolean VlcEngine::is_running()
{
	int isplaying = 0;
	
#ifdef USE_VLC_0_8_6_API
	isplaying = libvlc_playlist_isplaying(instance, &exception);
#else
//	isplaying = libvlc_media_player_isplaying(instance, &exception);
	isplaying = media_player != NULL;
#endif
	check_exception();
	
	return isplaying == 1;
}

void VlcEngine::mute(gboolean state)
{
	if (state != mute_state)
	{
		libvlc_audio_set_mute(instance, state, &exception);
		check_exception();
		mute_state = state;
	}
}

void VlcEngine::set_volume(gint newlevel)
{
	if (newlevel != volume)
	{
		libvlc_audio_set_volume(instance, newlevel, &exception);
		check_exception();
		volume = libvlc_audio_get_volume(instance, &exception);
		check_exception();
	}
}
