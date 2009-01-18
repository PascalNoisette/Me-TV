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
#include "lib_vlc_engine.h"
#include "exception.h"

#ifndef EXCLUDE_LIB_VLC_ENGINE

#define DEFAULT_VOLUME	130

typedef VLC_PUBLIC_API void (* function_libvlc_exception_init)( libvlc_exception_t *p_exception );
typedef VLC_PUBLIC_API libvlc_instance_t * (* function_libvlc_new)( int , const char *const *, libvlc_exception_t *);
typedef VLC_PUBLIC_API void (* function_libvlc_media_player_release)( libvlc_media_player_t * );
typedef VLC_PUBLIC_API void (* function_libvlc_audio_set_mute)( libvlc_instance_t *, int , libvlc_exception_t * );
typedef VLC_PUBLIC_API int (* function_libvlc_audio_get_volume)( libvlc_instance_t *, libvlc_exception_t * );
typedef VLC_PUBLIC_API void (* function_libvlc_release)( libvlc_instance_t * );
typedef VLC_PUBLIC_API int (* function_libvlc_exception_raised)( const libvlc_exception_t *p_exception );
typedef VLC_PUBLIC_API const char * (* function_libvlc_exception_get_message)( const libvlc_exception_t *p_exception );
typedef VLC_PUBLIC_API libvlc_media_player_t * (* function_libvlc_media_player_new)( libvlc_instance_t *, libvlc_exception_t * );
typedef VLC_PUBLIC_API void (* function_libvlc_media_player_set_drawable)( libvlc_media_player_t *, libvlc_drawable_t, libvlc_exception_t * );
typedef VLC_PUBLIC_API void (* function_libvlc_audio_set_volume)( libvlc_instance_t *, int, libvlc_exception_t *);
typedef VLC_PUBLIC_API libvlc_media_t * (* function_libvlc_media_new)(
                                   libvlc_instance_t *p_instance,
                                   const char * psz_mrl,
                                   libvlc_exception_t *p_e );
typedef VLC_PUBLIC_API void (* function_libvlc_media_add_option)(
                                   libvlc_media_t * p_md,
                                   const char * ppsz_options,
                                   libvlc_exception_t * p_e );
typedef VLC_PUBLIC_API void (* function_libvlc_media_player_set_media)( libvlc_media_player_t *, libvlc_media_t *, libvlc_exception_t * );
typedef VLC_PUBLIC_API void (* function_libvlc_media_release)(
                                   libvlc_media_t *p_meta_desc );
typedef VLC_PUBLIC_API void (* function_libvlc_media_player_play) ( libvlc_media_player_t *, libvlc_exception_t * );
typedef VLC_PUBLIC_API void (* function_libvlc_media_player_stop) ( libvlc_media_player_t *, libvlc_exception_t * );

function_libvlc_exception_init				symbol_libvlc_exception_init			= NULL;
function_libvlc_new							symbol_libvlc_new						= NULL;
function_libvlc_audio_set_mute				symbol_libvlc_audio_set_mute			= NULL;
function_libvlc_audio_get_volume			symbol_libvlc_audio_get_volume			= NULL;
function_libvlc_media_player_release		symbol_libvlc_media_player_release		= NULL;
function_libvlc_release						symbol_libvlc_release					= NULL;
function_libvlc_exception_raised			symbol_libvlc_exception_raised			= NULL;
function_libvlc_exception_get_message		symbol_libvlc_exception_get_message		= NULL;
function_libvlc_media_player_new			symbol_libvlc_media_player_new			= NULL;
function_libvlc_media_player_set_drawable	symbol_libvlc_media_player_set_drawable	= NULL;
function_libvlc_audio_set_volume			symbol_libvlc_audio_set_volume			= NULL;
function_libvlc_media_new					symbol_libvlc_media_new					= NULL;
function_libvlc_media_add_option			symbol_libvlc_media_add_option			= NULL;
function_libvlc_media_player_set_media		symbol_libvlc_media_player_set_media	= NULL;
function_libvlc_media_release				symbol_libvlc_media_release				= NULL;
function_libvlc_media_player_play			symbol_libvlc_media_player_play			= NULL;
function_libvlc_media_player_stop			symbol_libvlc_media_player_stop			= NULL;

LibVlcEngine::LibVlcEngine(int window_id) : window_id(window_id), module_lib_vlc("libvlc.so")
{
	const char * const argv[] = { 
		"-I", "dummy", 
		"--ignore-config", 
		"--no-osd" };

	if (!module_lib_vlc)
	{
		throw Exception(_("Failed to load VLC library"));
	}
	g_debug("VLC library loaded");
	
	symbol_libvlc_exception_init			= (function_libvlc_exception_init)				get_symbol("libvlc_exception_init");
	symbol_libvlc_new						= (function_libvlc_new)							get_symbol("libvlc_new");
	symbol_libvlc_audio_set_mute			= (function_libvlc_audio_set_mute)				get_symbol("libvlc_audio_set_mute");
	symbol_libvlc_audio_get_volume			= (function_libvlc_audio_get_volume)			get_symbol("libvlc_audio_get_volume");
	symbol_libvlc_media_player_release		= (function_libvlc_media_player_release)		get_symbol("libvlc_media_player_release");
	symbol_libvlc_release					= (function_libvlc_release)						get_symbol("libvlc_release");
	symbol_libvlc_exception_raised			= (function_libvlc_exception_raised)			get_symbol("libvlc_exception_raised");
	symbol_libvlc_exception_get_message		= (function_libvlc_exception_get_message)		get_symbol("libvlc_exception_get_message");
	symbol_libvlc_media_player_new			= (function_libvlc_media_player_new)			get_symbol("libvlc_media_player_new");
	symbol_libvlc_media_player_set_drawable	= (function_libvlc_media_player_set_drawable)	get_symbol("libvlc_media_player_set_drawable");
	symbol_libvlc_audio_set_volume			= (function_libvlc_audio_set_volume)			get_symbol("libvlc_audio_set_volume");
	symbol_libvlc_media_new					= (function_libvlc_media_new)					get_symbol("libvlc_media_new");
	symbol_libvlc_media_add_option			= (function_libvlc_media_add_option)			get_symbol("libvlc_media_add_option");
	symbol_libvlc_media_player_set_media	= (function_libvlc_media_player_set_media)		get_symbol("libvlc_media_player_set_media");
	symbol_libvlc_media_release				= (function_libvlc_media_release)				get_symbol("libvlc_media_release");
	symbol_libvlc_media_player_play			= (function_libvlc_media_player_play)			get_symbol("libvlc_media_player_play");
	symbol_libvlc_media_player_stop			= (function_libvlc_media_player_stop)			get_symbol("libvlc_media_player_stop");	

	symbol_libvlc_exception_init (&exception);
	check_exception();

	instance = symbol_libvlc_new (sizeof(argv) / sizeof(argv[0]), argv, &exception);
	check_exception();

	volume = symbol_libvlc_audio_get_volume(instance, &exception);
	check_exception();
}

LibVlcEngine::~LibVlcEngine()
{
	symbol_libvlc_media_player_release(media_player);
	media_player = NULL;
	symbol_libvlc_release(instance);
	instance = NULL;
}

void* LibVlcEngine::get_symbol(const Glib::ustring& symbol_name)
{
	void* result = NULL;
	
	if (!module_lib_vlc.get_symbol(symbol_name, result))
	{
		Glib::ustring message = Glib::ustring::compose(_("Failed to load symbol '%1' from VLC library"), symbol_name);
		throw Exception(message);
	}
	
	return result;
}

void LibVlcEngine::check_exception()
{
	if (symbol_libvlc_exception_raised(&exception))
	{
		throw Exception(symbol_libvlc_exception_get_message(&exception));
	}
}

void LibVlcEngine::play(const Glib::ustring& mrl)
{
	Application& application = get_application();

	media_player = symbol_libvlc_media_player_new (instance, &exception);
	check_exception();

	symbol_libvlc_media_player_set_drawable(media_player, window_id, &exception);
	check_exception();

	symbol_libvlc_audio_set_volume (instance, volume, &exception);
	check_exception();

	libvlc_media_t* media = symbol_libvlc_media_new(instance, mrl.c_str(), &exception);
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
			symbol_libvlc_media_add_option(media, process_option.c_str(), &exception);
			check_exception();
		}
	}

	symbol_libvlc_media_player_set_media (media_player, media, &exception);
	check_exception();

	symbol_libvlc_media_release(media);

	symbol_libvlc_media_player_play(media_player, &exception);
	check_exception();

	mute(false);
	set_volume(DEFAULT_VOLUME);
}

void LibVlcEngine::stop()
{
	if(media_player != NULL)
	{
		symbol_libvlc_media_player_stop(media_player, &exception);
	}
	
	check_exception();
}

gboolean LibVlcEngine::is_running()
{
	int isplaying = 0;
	
	isplaying = media_player != NULL;
	check_exception();
	
	return isplaying == 1;
}

void LibVlcEngine::mute(gboolean state)
{
	symbol_libvlc_audio_set_mute(instance, state, &exception);
	check_exception();
}

void LibVlcEngine::set_volume(gint newlevel)
{
	if (newlevel != volume)
	{
		symbol_libvlc_audio_set_volume(instance, newlevel, &exception);
		check_exception();
		volume = symbol_libvlc_audio_get_volume(instance, &exception);
		check_exception();
	}
}

#endif
