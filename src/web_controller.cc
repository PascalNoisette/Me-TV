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

void WebController::sample_action(WebRequest & request)
{
    request.code = MHD_HTTP_OK;
    request.content = "alive";
}
void WebController::get_channels_action(WebRequest & request)
{
    for (auto & channel: channel_manager.get_channels()) {
        std::ostringstream text;
        text << channel.channel_id << "\t" << channel.name << "\n";
        request.content += text.str();
    }
    request.code = MHD_HTTP_OK;
}
void WebController::dispatch(WebRequest & request)
{
    if (request.is(MHD_HTTP_METHOD_GET)) {
        if (request.match("/channel")) {
            return get_channels_action(request);
        }
    }
    return sample_action(request);
}
