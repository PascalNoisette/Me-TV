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

ChannelManager::ChannelManager()
{
	display_channel = NULL;
}

void ChannelManager::load()
{
	Data::Table table = get_application().get_schema().tables["channel"];
	Data::Connection connection;
	Data::TableAdapter adapter(connection, table);
	load(adapter);
}

void ChannelManager::load(Data::TableAdapter& adapter)
{
	Data::DataTable data_table = adapter.select_rows("", "sort_order");

	for (Data::Rows::iterator i = data_table.rows.begin(); i != data_table.rows.end(); i++)
	{
		Data::Row row = *i;
		
		Channel channel;

		channel.channel_id									= row["channel_id"].int_value;
		channel.name										= row["name"].string_value;
		channel.flags										= row["flags"].int_value;
		channel.sort_order									= row["sort_order"].int_value;
		channel.mrl											= row["mrl"].string_value;
		channel.service_id									= row["service_id"].int_value;
		channel.transponder.frontend_parameters.frequency	= row["frequency"].int_value;
		channel.transponder.frontend_parameters.inversion	= (fe_spectral_inversion)row["inversion"].int_value;
		
		if (channel.flags & CHANNEL_FLAG_DVB_T)
		{
			channel.transponder.frontend_parameters.u.ofdm.bandwidth				= (fe_bandwidth)row["bandwidth"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.code_rate_HP				= (fe_code_rate)row["code_rate_lp"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.code_rate_LP				= (fe_code_rate)row["code_rate_lp"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.constellation			= (fe_modulation)row["constellation"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.transmission_mode		= (fe_transmit_mode)row["transmission_mode"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.guard_interval			= (fe_guard_interval)row["guard_interval"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.hierarchy_information	= (fe_hierarchy)row["hierarchy_information"].int_value;
		}
		else if (channel.flags & CHANNEL_FLAG_DVB_C)
		{
			channel.transponder.frontend_parameters.u.qam.symbol_rate	= row["symbol_rate"].int_value;
			channel.transponder.frontend_parameters.u.qam.fec_inner		= (fe_code_rate)row["fec_inner"].int_value;
			channel.transponder.frontend_parameters.u.qam.modulation	= (fe_modulation)row["modulation"].int_value;
		}
		else if (channel.flags & CHANNEL_FLAG_DVB_S)
		{
			channel.transponder.frontend_parameters.u.qpsk.symbol_rate	= row["symbol_rate"].int_value;
			channel.transponder.frontend_parameters.u.qpsk.fec_inner	= (fe_code_rate)row["fec_inner"].int_value;
			channel.transponder.polarisation							= row["polarisation"].int_value;
		}
		else if (channel.flags & CHANNEL_FLAG_ATSC)
		{
			channel.transponder.frontend_parameters.u.vsb.modulation	= (fe_modulation)row["modulation"].int_value;
		}
		
		add_channel(channel);
	}
}

void ChannelManager::save()
{	
	Data::Table table = get_application().get_schema().tables["channel"];
	Data::DataTable data_table(table);

	ChannelList::iterator iterator = channels.begin();
	for (ChannelList::iterator i = channels.begin(); i != channels.end(); i++)
	{
		Channel& channel = *i;
		
		Data::Row row;
		row["channel_id"].int_value				= channel.channel_id;
		row["name"].string_value				= channel.name;
		row["flags"].int_value					= channel.flags;
		row["sort_order"].int_value				= channel.sort_order;
		row["mrl"].string_value					= channel.mrl;
		row["service_id"].int_value				= channel.service_id;
		row["frequency"].int_value				= channel.transponder.frontend_parameters.frequency;
		row["inversion"].int_value				= channel.transponder.frontend_parameters.inversion;

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
		data_table.rows.push_back(row);
	}	
	
	Data::Connection connection;
	Data::TableAdapter adapter(connection, table);
	adapter.replace(data_table);

	// Reload to make sure that we get the channel_ids updated
	load(adapter);
}

Channel* ChannelManager::find_channel(guint channel_id)
{
	Channel* channel = NULL;

	ChannelList::iterator iterator = channels.begin();
	while (iterator != channels.end() && channel == NULL)
	{
		Channel* current_channel = &(*iterator);
		if (current_channel->channel_id == channel_id)
		{
			channel = current_channel;
		}
		
		iterator++;
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
	g_message("Setting display channel to '%s' (%d)", channel.name.c_str(), channel.channel_id);
	display_channel = find_channel(channel.channel_id);
	if (display_channel == NULL)
	{
		throw Exception(_("Failed to set display channel: channel not found"));
	}
	else
	{
		signal_display_channel_changed(*display_channel);
	}
}

void ChannelManager::add_channels(const ChannelList& c)
{
	ChannelList::const_iterator iterator = c.begin();
	while (iterator != c.end())
	{
		const Channel& channel = *iterator;
		channels.push_back(channel);
		iterator++;
	}
}

void ChannelManager::add_channel(const Channel& channel)
{
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
	channels.clear();
	g_debug("Channels cleared");
}

Channel* ChannelManager::find_channel(guint frequency, guint service_id)
{
	Channel* result = NULL;
	
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
	if (channels.size() == 0)
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
	if (channels.size() == 0)
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
