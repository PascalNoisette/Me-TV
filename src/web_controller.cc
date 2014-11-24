/*
 * Copyright © 2014 Pascal Noisette
 * 
 * File:   web_controller.cc
 * Author: Pascal Noisette
 * Created: November 15, 2014, 10:00 AM
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

#include <glibmm.h>
#include "web_controller.h"
#include "application.h"
#include <jsoncpp/json/json.h>

void WebController::sample_action(WebRequest & request)
{
    request.code = MHD_HTTP_OK;
    request.content = "alive";
}
void WebController::get_channels_action(WebRequest & request)
{
    Json::Value json;
    for (Channel & channel: channel_manager.get_channels()) {
        Json::Value json_channel;
        json_channel["channel_id"] = channel.channel_id;
        json_channel["name"] = channel.name.c_str();
        json.append(json_channel);
    }
    request.content = json.toStyledString();
    request.content_type = "application/json";
    request.code = MHD_HTTP_OK;
}
void WebController::get_recordings_action(WebRequest & request)
{
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
        json.append(json_recording);
    }
    request.content = json.toStyledString();
    request.content_type = "application/json";
    request.code = MHD_HTTP_OK;
}
void WebController::echo_action(WebRequest & request)
{
    request.content += request.method + "\t" + request.url + "\n";
    for (std::map<Glib::ustring, Glib::ustring>::iterator iter = request.params.begin(); iter != request.params.end(); ++iter) {
        request.content += iter->first + "\t" + iter->second + "\n";
    }
    request.code = MHD_HTTP_OK;
}
void WebController::dispatch(WebRequest & request)
{
    if (request.is(MHD_HTTP_METHOD_GET)) {
        if (request.match("/channel")) {
            return get_channels_action(request);
        }
        if (request.match("/recording")) {
            return get_recordings_action(request);
        }
    }
    if (request.match("/echo")) {
        return echo_action(request);
    }
    return sample_action(request);
}
