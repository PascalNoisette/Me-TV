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

#include "channel_manager.h"
#include "exception.h"
#include "me-tv-i18n.h"
#include "application.h"

#define MAX_CHANNELS	10000

class LockLogger
{
private:
	Glib::ustring description;
	Glib::RecMutex& mutex;
public:
	LockLogger(Glib::RecMutex& mutex, const Glib::ustring& description)
		: description(description), mutex(mutex)
	{
		//g_debug(">>> Locking (%s)", description.c_str());
		mutex.lock();
	}

	~LockLogger()
	{
		//g_debug("<<< UnLocking (%s)", description.c_str());
		mutex.unlock();
	}
};

ChannelManager::ChannelManager()
{
	display_channel = NULL;
	g_static_rec_mutex_init(mutex.gobj());
}

void ChannelManager::load(Data::Connection& connection)
{
	LockLogger lock(mutex, __PRETTY_FUNCTION__);

	Application& application = get_application();

	Data::Table table_channel			= application.get_schema().tables["channel"];
	Data::Table table_epg_event			= application.get_schema().tables["epg_event"];
	Data::Table table_epg_event_text	= application.get_schema().tables["epg_event_text"];

	Data::TableAdapter adapter_channel(connection, table_channel);
	Data::TableAdapter adapter_epg_event(connection, table_epg_event);
	Data::TableAdapter adapter_epg_event_text(connection, table_epg_event_text);

	Data::DataTable data_table_channels = adapter_channel.select_rows("", "sort_order");

	g_debug("Loading channels ...");
	for (Data::Rows::iterator i = data_table_channels.rows.begin(); i != data_table_channels.rows.end(); i++)
	{
		Data::Row row_channel = *i;
		
		Channel channel;

		channel.channel_id									= row_channel["channel_id"].int_value;
		channel.name										= row_channel["name"].string_value;
		channel.flags										= row_channel["flags"].int_value;
		channel.sort_order									= row_channel["sort_order"].int_value;
		channel.mrl											= row_channel["mrl"].string_value;
		channel.service_id									= row_channel["service_id"].int_value;
		channel.transponder.frontend_parameters.frequency	= row_channel["frequency"].int_value;
		channel.transponder.frontend_parameters.inversion	= (fe_spectral_inversion)row_channel["inversion"].int_value;
		
		g_debug("Loading channel '%s'", channel.name.c_str());

		if (channel.flags & CHANNEL_FLAG_DVB_T)
		{
			channel.transponder.frontend_parameters.u.ofdm.bandwidth				= (fe_bandwidth)row_channel["bandwidth"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.code_rate_HP				= (fe_code_rate)row_channel["code_rate_hp"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.code_rate_LP				= (fe_code_rate)row_channel["code_rate_lp"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.constellation			= (fe_modulation)row_channel["constellation"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.transmission_mode		= (fe_transmit_mode)row_channel["transmission_mode"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.guard_interval			= (fe_guard_interval)row_channel["guard_interval"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.hierarchy_information	= (fe_hierarchy)row_channel["hierarchy_information"].int_value;
		}
		else if (channel.flags & CHANNEL_FLAG_DVB_C)
		{
			channel.transponder.frontend_parameters.u.qam.symbol_rate	= row_channel["symbol_rate"].int_value;
			channel.transponder.frontend_parameters.u.qam.fec_inner		= (fe_code_rate)row_channel["fec_inner"].int_value;
			channel.transponder.frontend_parameters.u.qam.modulation	= (fe_modulation)row_channel["modulation"].int_value;
		}
		else if (channel.flags & CHANNEL_FLAG_DVB_S)
		{
			channel.transponder.frontend_parameters.u.qpsk.symbol_rate	= row_channel["symbol_rate"].int_value;
			channel.transponder.frontend_parameters.u.qpsk.fec_inner	= (fe_code_rate)row_channel["fec_inner"].int_value;
			channel.transponder.polarisation							= row_channel["polarisation"].int_value;
		}
		else if (channel.flags & CHANNEL_FLAG_ATSC)
		{
			channel.transponder.frontend_parameters.u.vsb.modulation	= (fe_modulation)row_channel["modulation"].int_value;
		}
		
		Glib::ustring clause = Glib::ustring::compose("channel_id = %1", channel.channel_id);
		Data::DataTable data_table_epg_event = adapter_epg_event.select_rows(clause, "start_time");		
		for (Data::Rows::iterator j = data_table_epg_event.rows.begin(); j != data_table_epg_event.rows.end(); j++)
		{
			Data::Row row_epg_event = *j;
			EpgEvent epg_event;
			
			epg_event.epg_event_id	= row_epg_event["epg_event_id"].int_value;
			epg_event.channel_id	= channel.channel_id;
			epg_event.event_id		= row_epg_event["event_id"].int_value;
			epg_event.start_time	= row_epg_event["start_time"].int_value;
			epg_event.duration		= row_epg_event["duration"].int_value;
			
			Glib::ustring clause = Glib::ustring::compose("epg_event_id = %1", epg_event.epg_event_id);
			Data::DataTable data_table_epg_event_text = adapter_epg_event_text.select_rows(clause);
			for (Data::Rows::iterator k = data_table_epg_event_text.rows.begin(); k != data_table_epg_event_text.rows.end(); k++)
			{
				Data::Row row_epg_event_text = *k;
				EpgEventText epg_event_text;
				
				epg_event_text.epg_event_text_id	= row_epg_event_text["epg_event_text_id"].int_value;
				epg_event_text.epg_event_id			= epg_event.epg_event_id;
				epg_event_text.language				= row_epg_event_text["language"].string_value;
				epg_event_text.title				= row_epg_event_text["title"].string_value;
				epg_event_text.description			= row_epg_event_text["description"].string_value;
								
				epg_event.texts.push_back(epg_event_text);
			}			
			
			channel.epg_events.add_epg_event(epg_event);
		}
		
		add_channel(channel);
	}
	g_debug("Channels loaded");
}

void ChannelManager::save(Data::Connection& connection)
{
	Application& application = get_application();
	
	mutex.lock();

	Data::Table table_channel = application.get_schema().tables["channel"];
	Data::TableAdapter adapter_channel(connection, table_channel);
	Data::DataTable data_table_channel(table_channel);
		
	g_debug("Saving channels");
	
	// Update sort_order = 0 to flag unused channels for removal later
	adapter_channel.update_rows("sort_order = 0");
		
	int channel_count = 0;
	for (ChannelList::iterator i = channels.begin(); i != channels.end(); i++)
	{
		Channel& channel = *i;
		
		g_debug("Saving channel '%s'", channel.name.c_str());

		Data::Row row;
		
		row.auto_increment = &(channel.channel_id);
		
		row["channel_id"].int_value		= channel.channel_id;
		row["name"].string_value		= channel.name;
		row["flags"].int_value			= channel.flags;
		row["sort_order"].int_value		= ++channel_count;
		row["mrl"].string_value			= channel.mrl;
		row["service_id"].int_value		= channel.service_id;
		row["frequency"].int_value		= channel.transponder.frontend_parameters.frequency;
		row["inversion"].int_value		= channel.transponder.frontend_parameters.inversion;

		if (channel.flags & CHANNEL_FLAG_DVB_T)
		{
			row["bandwidth"].int_value				= channel.transponder.frontend_parameters.u.ofdm.bandwidth;
			row["code_rate_hp"].int_value			= channel.transponder.frontend_parameters.u.ofdm.code_rate_HP;
			row["code_rate_lp"].int_value			= channel.transponder.frontend_parameters.u.ofdm.code_rate_LP;
			row["constellation"].int_value			= channel.transponder.frontend_parameters.u.ofdm.constellation;
			row["transmission_mode"].int_value		= channel.transponder.frontend_parameters.u.ofdm.transmission_mode;
			row["guard_interval"].int_value			= channel.transponder.frontend_parameters.u.ofdm.guard_interval;
			row["hierarchy_information"].int_value	= channel.transponder.frontend_parameters.u.ofdm.hierarchy_information;
		}
		else if (channel.flags & CHANNEL_FLAG_DVB_C)
		{
			row["symbol_rate"].int_value			= channel.transponder.frontend_parameters.u.qam.symbol_rate;
			row["fec_inner"].int_value				= channel.transponder.frontend_parameters.u.qam.fec_inner;
			row["modulation"].int_value				= channel.transponder.frontend_parameters.u.qam.modulation;
		}
		else if (channel.flags & CHANNEL_FLAG_DVB_S)
		{
			row["symbol_rate"].int_value			= channel.transponder.frontend_parameters.u.qpsk.symbol_rate;
			row["fec_inner"].int_value				= channel.transponder.frontend_parameters.u.qpsk.fec_inner;
			row["polarisation"].int_value			= channel.transponder.polarisation;
		}
		else if (channel.flags & CHANNEL_FLAG_ATSC)
		{
			row["modulation"].int_value				= channel.transponder.frontend_parameters.u.vsb.modulation;
		}
		data_table_channel.rows.push_back(row);

		g_debug("Channel '%s' saved", channel.name.c_str());
	}
	adapter_channel.replace_rows(data_table_channel);
	
	// Delete channels that are not used
	adapter_channel.delete_rows("sort_order = 0");

	ChannelList channels_copy = channels;

	g_debug("Channels saved");
	mutex.unlock();

	for (ChannelList::iterator i = channels_copy.begin(); i != channels_copy.end(); i++)
	{
		Channel& channel = *i;
		channel.epg_events.save(connection, channel.channel_id);
	}
}

Channel* ChannelManager::find_channel(guint channel_id)
{
	Channel* channel = NULL;

	g_debug("*** find_channel(%d)", channel_id);
	LockLogger lock(mutex, __PRETTY_FUNCTION__);
	ChannelList::iterator iterator = channels.begin();
	while (iterator != channels.end() && channel == NULL)
	{
		Channel* current_channel = &(*iterator);
		if (current_channel->channel_id == channel_id)
		{
			g_debug("*** find_channel(%d) FOUND", channel_id);
			channel = current_channel;
		}
		
		iterator++;
	}

	if (channel == NULL)
	{
		g_debug("*** find_channel(%d) NOT FOUND", channel_id);
	}
	else
	{
		g_debug("*** find_channel(%d) RETURNING", channel_id);
	}
	
	return channel;
}

Channel& ChannelManager::get_channel(guint channel_id)
{
	Channel* channel = find_channel(channel_id);

	if (channel == NULL)
	{
		throw Exception(Glib::ustring::compose(_("Channel '%1' not found"), channel_id));
	}

	return *channel;
}

void ChannelManager::set_display_channel(guint channel_id)
{
	set_display_channel(get_channel(channel_id));
}

void ChannelManager::set_display_channel(const Channel& channel)
{
	LockLogger lock(mutex, __PRETTY_FUNCTION__);

	g_message("Setting display channel to '%s' (%d)", channel.name.c_str(), channel.channel_id);
	Channel* new_display_channel = find_channel(channel.channel_id);
	if (new_display_channel == NULL)
	{
		throw Exception(_("Failed to set display channel: channel not found"));
	}
	display_channel = new_display_channel;
}

void ChannelManager::add_channels(const ChannelList& c)
{
	LockLogger lock(mutex, __PRETTY_FUNCTION__);

	ChannelList::const_iterator iterator = c.begin();
	while (iterator != c.end())
	{
		const Channel& channel = *iterator;
		add_channel(channel);
		iterator++;
	}
}

void ChannelManager::add_channel(const Channel& channel)
{
	LockLogger lock(mutex, __PRETTY_FUNCTION__);

	if (channels.size() >= MAX_CHANNELS)
	{
		Glib::ustring message = Glib::ustring::compose(ngettext(
			"Failed to add channel: You cannot have more than %1 channel",
			"Failed to add channel: You cannot have more than %1 channels",
			MAX_CHANNELS), MAX_CHANNELS);
		throw Exception(message);
	}
	channels.push_back(channel);
	g_debug("Channel '%s' added", channel.name.c_str());
}

ChannelList& ChannelManager::get_channels()
{
	return channels;
}

const ChannelList& ChannelManager::get_channels() const
{
	return channels;
}

Channel* ChannelManager::get_display_channel()
{
	return display_channel;
}

void ChannelManager::clear()
{
	LockLogger lock(mutex, __PRETTY_FUNCTION__);
	channels.clear();
	g_debug("Channels cleared");
}

Channel* ChannelManager::find_channel(guint frequency, guint service_id)
{
	Channel* result = NULL;
	
	LockLogger lock(mutex, __PRETTY_FUNCTION__);
	for (ChannelList::iterator iterator = channels.begin(); iterator != channels.end() && result == NULL; iterator++)
	{
		Channel& channel = *iterator;
		if (channel.get_transponder_frequency() == frequency && channel.service_id == service_id)
		{
			result = &channel;
		}
	}
	
	return result;
}

void ChannelManager::set_channels(const ChannelList& new_channels)
{
	g_debug("Setting channels");
	
	LockLogger lock(mutex, __PRETTY_FUNCTION__);

	guint display_channel_frequency = 0;
	guint display_channel_service_id = 0;

	if (display_channel != NULL)
	{
		display_channel_frequency = display_channel->get_transponder_frequency();
		display_channel_service_id = display_channel->service_id;
		display_channel = NULL; // Will be destroyed by clear, so flag it
	}
	
	clear();
	add_channels(new_channels);
	
	// Force save to get updated channel IDs
	save(get_application().connection);
	
	if (display_channel_frequency != 0)
	{
		Channel* channel = find_channel(display_channel_frequency, display_channel_service_id);
		if (channel == NULL && channels.size() > 0)
		{
			channel = &(*(channels.begin()));
		}
		
		if (channel != NULL)
		{
			set_display_channel(*channel);
		}
	}
	g_debug("Finished setting channels");
}

void ChannelManager::next_channel()
{
	LockLogger lock(mutex, __PRETTY_FUNCTION__);

	if (channels.empty())
	{
		throw Exception(_("No channels"));
	}
	else if (display_channel == NULL)
	{
		set_display_channel(*(channels.begin()));
	}
	else
	{
		gboolean done = false;
		
		ChannelList::iterator iterator = channels.begin();
		while (iterator != channels.end() && !done)
		{
			Channel& channel = *iterator;
			if (channel.get_transponder_frequency() == display_channel->get_transponder_frequency() && channel.service_id == display_channel->service_id)
			{
				iterator++;
				if (iterator != channels.end())
				{
					set_display_channel(*iterator);
				}
				done = true;
			}
			else
			{
				iterator++;
			}			
		}
	}
}

void ChannelManager::previous_channel()
{
	LockLogger lock(mutex, __PRETTY_FUNCTION__);

	if (channels.empty())
	{
		throw Exception(_("No channels"));
	}
	else if (display_channel == NULL)
	{
		ChannelList::iterator iterator = channels.end();
		iterator--;
		set_display_channel(*iterator);
	}
	else
	{
		gboolean done = false;
		
		ChannelList::iterator iterator = channels.begin();
		ChannelList::iterator previous_iterator = iterator;
		while (iterator != channels.end() && !done)
		{
			Channel& channel = *iterator;
			if (channel.get_transponder_frequency() == display_channel->get_transponder_frequency() && channel.service_id == display_channel->service_id)
			{
				if (previous_iterator != iterator)
				{
					set_display_channel(*previous_iterator);
				}
				done = true;
			}
			else
			{
				previous_iterator = iterator;
				iterator++;
			}
		}
	}
}
