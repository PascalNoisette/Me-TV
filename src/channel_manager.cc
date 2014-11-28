/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright (C) 2011 Michael Lamothe
 *  Copyright © 2014  Russel Winder
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "channel_manager.h"
#include "exception.h"
#include "me-tv-i18n.h"
#include "application.h"

constexpr auto MAX_CHANNELS = 10000;

void ChannelManager::initialise() {
	dirty = true;
}

gboolean ChannelManager::is_dirty() {
	gboolean result = dirty;
	Glib::Threads::RecMutex::Lock lock(mutex);
	if (!result) {
		for (auto const channel: channels) {
			if (channel.epg_events.is_dirty()) {
				result = true;
				break;
			}
		}
	}
	return result;
}

void ChannelManager::load(Data::Connection & connection) {
	Glib::Threads::RecMutex::Lock lock(mutex);
	Application & application = get_application();
	Data::Table table_channel = application.get_schema().tables["channel"];
	Data::TableAdapter adapter_channel(connection, table_channel);
	Data::DataTable data_table_channels = adapter_channel.select_rows("", "sort_order");
	g_debug("Loading channels ...");
	for (auto row_channel: data_table_channels.rows) {
		Channel channel;
		channel.channel_id = row_channel["channel_id"].int_value;
		channel.name = row_channel["name"].string_value;
		channel.transponder.frontend_type = (fe_type_t)row_channel["type"].int_value;
		channel.sort_order = row_channel["sort_order"].int_value;
		channel.mrl = row_channel["mrl"].string_value;
		channel.service_id = row_channel["service_id"].int_value;
		channel.transponder.frontend_parameters.frequency = row_channel["frequency"].int_value;
		channel.transponder.frontend_parameters.inversion = (fe_spectral_inversion)row_channel["inversion"].int_value;
		g_debug("Loading channel '%s'", channel.name.c_str());
		switch(channel.transponder.frontend_type) {
		case FE_OFDM:
			channel.transponder.frontend_parameters.u.ofdm.bandwidth = (fe_bandwidth)row_channel["bandwidth"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.code_rate_HP = (fe_code_rate)row_channel["code_rate_hp"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.code_rate_LP = (fe_code_rate)row_channel["code_rate_lp"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.constellation = (fe_modulation)row_channel["constellation"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.transmission_mode = (fe_transmit_mode)row_channel["transmission_mode"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.guard_interval = (fe_guard_interval)row_channel["guard_interval"].int_value;
			channel.transponder.frontend_parameters.u.ofdm.hierarchy_information = (fe_hierarchy)row_channel["hierarchy_information"].int_value;
			break;
		case FE_QAM:
			channel.transponder.frontend_parameters.u.qam.symbol_rate = row_channel["symbol_rate"].int_value;
			channel.transponder.frontend_parameters.u.qam.fec_inner = (fe_code_rate)row_channel["fec_inner"].int_value;
			channel.transponder.frontend_parameters.u.qam.modulation = (fe_modulation)row_channel["modulation"].int_value;
			break;
		case FE_QPSK:
			channel.transponder.frontend_parameters.u.qpsk.symbol_rate = row_channel["symbol_rate"].int_value;
			channel.transponder.frontend_parameters.u.qpsk.fec_inner = (fe_code_rate)row_channel["fec_inner"].int_value;
			channel.transponder.polarisation = row_channel["polarisation"].int_value;
			break;
		case FE_ATSC:
			channel.transponder.frontend_parameters.u.vsb.modulation = (fe_modulation)row_channel["modulation"].int_value;
			break;
		default:
			throw Exception(_("Unknown frontend type"));
		}
		channel.epg_events.load(connection, channel.channel_id);
		add_channel(channel);
	}
	g_debug("Channels loaded");
}

void ChannelManager::save(Data::Connection & connection) {
	Application & application = get_application();
	Data::Table table_channel = application.get_schema().tables["channel"];
	Data::TableAdapter adapter_channel(connection, table_channel);
	Data::DataTable data_table_channel(table_channel);
	g_debug("Saving channels");
	// Update sort_order = 0 to flag unused channels for removal later
	adapter_channel.update_rows("sort_order = 0");
	Glib::Threads::RecMutex::Lock lock(mutex);
	int channel_count = 0;
	for (auto channel: channels) {
		g_debug("Saving channel '%s'", channel.name.c_str());
		Data::Row row;
		row.auto_increment = &(channel.channel_id);
		row["channel_id"].int_value = channel.channel_id;
		row["name"].string_value = channel.name;
		row["type"].int_value = channel.transponder.frontend_type;
		row["sort_order"].int_value = ++channel_count;
		row["mrl"].string_value = channel.mrl;
		row["service_id"].int_value = channel.service_id;
		row["frequency"].int_value = channel.transponder.frontend_parameters.frequency;
		row["inversion"].int_value = channel.transponder.frontend_parameters.inversion;
		switch(channel.transponder.frontend_type) {
		case FE_OFDM:
			row["bandwidth"].int_value = channel.transponder.frontend_parameters.u.ofdm.bandwidth;
			row["code_rate_hp"].int_value = channel.transponder.frontend_parameters.u.ofdm.code_rate_HP;
			row["code_rate_lp"].int_value = channel.transponder.frontend_parameters.u.ofdm.code_rate_LP;
			row["constellation"].int_value = channel.transponder.frontend_parameters.u.ofdm.constellation;
			row["transmission_mode"].int_value = channel.transponder.frontend_parameters.u.ofdm.transmission_mode;
			row["guard_interval"].int_value = channel.transponder.frontend_parameters.u.ofdm.guard_interval;
			row["hierarchy_information"].int_value = channel.transponder.frontend_parameters.u.ofdm.hierarchy_information;
			break;
		case FE_QAM:
			row["symbol_rate"].int_value = channel.transponder.frontend_parameters.u.qam.symbol_rate;
			row["fec_inner"].int_value = channel.transponder.frontend_parameters.u.qam.fec_inner;
			row["modulation"].int_value = channel.transponder.frontend_parameters.u.qam.modulation;
			break;
		case FE_QPSK:
			row["symbol_rate"].int_value = channel.transponder.frontend_parameters.u.qpsk.symbol_rate;
			row["fec_inner"].int_value = channel.transponder.frontend_parameters.u.qpsk.fec_inner;
			row["polarisation"].int_value = channel.transponder.polarisation;
			break;
		case FE_ATSC:
			row["modulation"].int_value = channel.transponder.frontend_parameters.u.vsb.modulation;
			break;
		default:
			throw Exception(_("Unknown frontend type"));
		}
		data_table_channel.rows.push_back(row);
		g_debug("Channel '%s' saved", channel.name.c_str());
	}
	adapter_channel.replace_rows(data_table_channel);
	// Delete channels that are not used
	adapter_channel.delete_rows("sort_order = 0");
	Data::Table table_scheduled_recording = application.get_schema().tables["scheduled_recording"];
	Data::Table table_epg_event = application.get_schema().tables["epg_event"];
	Data::Table table_epg_event_text = application.get_schema().tables["epg_event_text"];
	// Delete orphaned scheduled recordings
	Data::TableAdapter adapter_scheduled_recording(connection, table_scheduled_recording);
	adapter_scheduled_recording.delete_rows("CHANNEL_ID NOT IN (SELECT CHANNEL_ID FROM CHANNEL)");
	// Delete orphaned epg_events
	Data::TableAdapter adapter_epg_event(connection, table_epg_event);
	adapter_epg_event.delete_rows("CHANNEL_ID NOT IN (SELECT CHANNEL_ID FROM CHANNEL)");
	g_debug("Channels saved");
	for (auto channel: channels) {
		g_debug("Saving EPG events for '%s'", channel.name.c_str());
		channel.epg_events.save(
			connection,
			channel.channel_id);
	}
	g_debug("Deleting old EPG events");
	Glib::ustring clause_epg_event = Glib::ustring::compose("(START_TIME+DURATION)<%1", get_local_time() - 36000);
	adapter_epg_event.delete_rows(clause_epg_event);
	// Delete orphaned epg_events_texts
	Data::TableAdapter adapter_epg_event_text(connection, table_epg_event_text);
	adapter_epg_event_text.delete_rows("EPG_EVENT_ID NOT IN (SELECT EPG_EVENT_ID FROM EPG_EVENT)");
	dirty = false;
	g_debug("EPG events saved");
}

Channel * ChannelManager::find_channel(guint channel_id) {
	Channel * result {0};
	Glib::Threads::RecMutex::Lock lock(mutex);
	for (auto & channel: channels) {
		if (channel.channel_id == channel_id) {
			result = &channel;
			break;
		}
	}
	return result;
}

Channel & ChannelManager::get_channel_by_index(guint index) {
	Glib::Threads::RecMutex::Lock lock(mutex);
	if (index >= channels.size()) {
		throw Exception(_("Invalid channel index"));
	}
	return channels[index];
}

Channel & ChannelManager::get_channel_by_id(guint channel_id) {
	Glib::Threads::RecMutex::Lock lock(mutex);
	Channel* channel = find_channel(channel_id);
	if (channel == NULL) {
		throw Exception(Glib::ustring::compose(_("Channel '%1' not found"), channel_id));
	}
	return *channel;
}

void ChannelManager::add_channels(ChannelArray const & c) {
	Glib::Threads::RecMutex::Lock lock(mutex);
	for (auto const channel: c) {
		add_channel(channel);
	}
}

void ChannelManager::add_channel(Channel const & channel) {
	Glib::Threads::RecMutex::Lock lock(mutex);
	if (channels.size() >= MAX_CHANNELS) {
		Glib::ustring message = Glib::ustring::compose(ngettext(
			"Failed to add channel: You cannot have more than 1 channel",
			"Failed to add channel: You cannot have more than %1 channels",
			MAX_CHANNELS), MAX_CHANNELS);
		throw Exception(message);
	}
	for (auto const c: channels) {
		if (channel.channel_id != 0 && c.channel_id == channel.channel_id) {
			throw Exception("Failed to add channel: channel id already exists");
		}
	}
	channels.push_back(channel);
	g_debug("Channel '%s' added", channel.name.c_str());
}

ChannelArray & ChannelManager::get_channels() {
	return channels;
}

ChannelArray const & ChannelManager::get_channels() const {
	return channels;
}

void ChannelManager::clear() {
	Glib::Threads::RecMutex::Lock lock(mutex);
	channels.clear();
	g_debug("Channels cleared");
}

Channel * ChannelManager::find_channel(guint frequency, guint service_id) {
	Channel * result {0};
	Glib::Threads::RecMutex::Lock lock(mutex);
	for (auto & channel: channels) {
		if (channel.get_transponder_frequency() == frequency && channel.service_id == service_id) {
			result = &channel;
			break;
		}
	}
	return result;
}

void ChannelManager::set_channels(ChannelArray const & new_channels) {
	g_debug("Setting channels");
	Glib::Threads::RecMutex::Lock lock(mutex);
	clear();
	add_channels(new_channels);
	// Force save to get updated channel IDs
	save(get_application().connection);
	g_debug("Finished setting channels");
}

void ChannelManager::prune_epg() {
	Glib::Threads::RecMutex::Lock lock(mutex);
	// TODO: there must be a better way…
	ChannelArray::iterator iterator = channels.begin();
	while (iterator != channels.end()) {
		Channel& channel = *iterator;
		channel.epg_events.prune();
		iterator++;
	}
}
