/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright © 2014 Pascal Noisette
 *
 *  Author: Pascal Noisette
 *  Created: November 15, 2014, 10:00 AM
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

#include <glibmm.h>
#include "web_controller.h"
#include "application.h"
#include <jsoncpp/json/json.h>

void WebController::sample_action(WebRequest & request) {
	request.code = MHD_HTTP_OK;
	request.content = "alive";
}

void WebController::get_channels_action(WebRequest & request) {
	Json::Value json;
	for (Channel & channel: channel_manager.get_channels()) {
		Json::Value json_channel;
		json_channel["channel_id"] = channel.channel_id;
		json_channel["name"] = channel.name.c_str();
		json.append(json_channel);
	}
	request.content = json.toStyledString();
	request.headers[MHD_HTTP_HEADER_CONTENT_TYPE] = "application/json";
	request.code = MHD_HTTP_OK;
}

void WebController::get_recordings_action(WebRequest & request) {
	Json::Value json;
	for (ScheduledRecording & recording: scheduled_recording_manager.scheduled_recordings) {
		Json::Value json_recording;
		json_recording["scheduled_recording_id"] = recording.scheduled_recording_id;
		json_recording["description"] = recording.description.c_str();
		json_recording["recurring_type"] = recording.recurring_type;
		json_recording["action_after"] = recording.action_after;
		json_recording["channel_id"] = recording.channel_id;
		json_recording["start_time"] = (unsigned int) (recording.start_time);
		json_recording["duration"] = recording.duration;
		json_recording["device"] = recording.device.c_str();
		json.append(json_recording);
	}
	request.content = json.toStyledString();
	request.headers[MHD_HTTP_HEADER_CONTENT_TYPE] = "application/json";
	request.code = MHD_HTTP_OK;
}

void WebController::post_recordings_action(WebRequest & request) {
	ScheduledRecording recording;
	recording.device = request.params["device"];
	recording.scheduled_recording_id = atoi(request.params["scheduled_recording_id"].c_str());
	recording.recurring_type = atoi(request.params["recurring_type"].c_str());
	recording.action_after = atoi(request.params["action_after"].c_str());
	recording.description = request.params["description"];
	recording.channel_id = atoi(request.params["channel_id"].c_str());
	recording.start_time = atoi(request.params["start_time"].c_str());
	recording.duration = atoi(request.params["duration"].c_str());
	Json::Value json;
	try {
		scheduled_recording_manager.set_scheduled_recording(recording);
		json["type"] = "success";
		json["msg"] = "OK";
	}
	catch (Exception e) {
		json["type"] = "danger";
		json["msg"] = e.what().c_str();
	}
	request.content = json.toStyledString();
	request.headers[MHD_HTTP_HEADER_CONTENT_TYPE] = "application/json";
	request.code = MHD_HTTP_OK;
}

void WebController::delete_recording_action(WebRequest & request) {
	Json::Value json;
	guint scheduled_recording_id;
	try {
		scheduled_recording_id = atoi(request.params["scheduled_recording_id"].c_str());
		g_debug("Removing %d", scheduled_recording_id);
		scheduled_recording_manager.remove_scheduled_recording(scheduled_recording_id);
		json["type"] = "success";
		json["msg"] = "OK";
	}
	catch (Exception e) {
		json["type"] = "danger";
		json["msg"] = e.what().c_str();
	}
	request.content = json.toStyledString();
	request.headers[MHD_HTTP_HEADER_CONTENT_TYPE] = "application/json";
	request.code = MHD_HTTP_OK;
}

void WebController::echo_action(WebRequest & request) {
	request.content += request.method + "\t" + request.url + "\n";
	for (std::map<Glib::ustring, Glib::ustring>::iterator iter = request.params.begin(); iter != request.params.end(); ++iter) {
		request.content += iter->first + "\t" + iter->second + "\n";
	}
	request.code = MHD_HTTP_OK;
}

Glib::ustring WebController::secure_filename(Glib::ustring file) {
	Glib::ustring::size_type pos = 0;
	Glib::ustring find = "..";
	Glib::ustring replace = ".";
	while((pos = file.find(find, pos)) != Glib::ustring::npos) {
		file.replace(pos, find.size(), replace);
		pos = 0;
	}
	return file;
}

void WebController::www_action(WebRequest & request) {
	Glib::ustring file = request.url;
	if (file == "/") {
		file = "/index.html";
	}
	request.download_file = PACKAGE_DATA_DIR"/html" + secure_filename(file);
	request.code = MHD_HTTP_OK;
}

void WebController::translate_action(WebRequest & request)
{
    Json::Value wrapper;
    Json::Value json;
    Json::Value translation;
    for (std::map<Glib::ustring, Glib::ustring>::iterator iter = request.params.begin(); iter != request.params.end(); ++iter) {
        translation[iter->second.c_str()]=_(iter->second.c_str());
    }
    json["translation"] =translation;
    wrapper.append(json);
    request.content = wrapper.toStyledString();
    request.headers[MHD_HTTP_HEADER_CONTENT_TYPE] = "application/json; charset=utf-8";
    request.code = MHD_HTTP_OK;
}

void WebController::dispatch(WebRequest & request) {
	if (request.is(MHD_HTTP_METHOD_GET)) {
		if (request.match("/channel")) {
			return get_channels_action(request);
		}
		if (request.match("/recording")) {
			return get_recordings_action(request);
		}
		if (request.match("/echo")) {
			return echo_action(request);
		}
                if (request.match("/translate")) {
                    return translate_action(request);
                }
		return www_action(request);
	}
	if (request.is(MHD_HTTP_METHOD_POST)) {
		if (request.match("/recording")) {
			return post_recordings_action(request);
		}
	}
	if (request.is(MHD_HTTP_METHOD_DELETE)) {
		if (request.match("/recording")) {
			return delete_recording_action(request);
		}
	}
	if (request.match("/echo")) {
		return echo_action(request);
	}
	return sample_action(request);
}