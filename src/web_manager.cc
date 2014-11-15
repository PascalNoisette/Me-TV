/*
 * Copyright © 2014 Pascal Noisette
 * 
 * File:   web_manager.cc
 * Author: Pascal Noisette
 * Created: November 11, 2014, 12:47 PM
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
#include "web_manager.h"

int WebManager::handler(void * cls, struct MHD_Connection * connection, const char * url, const char * method, const char * version, const char * upload_data, size_t * upload_data_size, void ** ptr) {
    if (*ptr == NULL) {
        *ptr = (void*) !NULL;
        return MHD_YES;
    } else {
        *ptr = NULL;
        char content[]  = "alive";
        Glib::ustring contentString = content;
        struct MHD_Response * response = MHD_create_response_from_data(contentString.length(), content, MHD_NO, MHD_NO);
        int ret = MHD_queue_response(connection, 200, response);
        MHD_destroy_response(response);

        return ret;
    }
}

void WebManager::start() {
    g_debug("WebManager start");
    daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, WEB_PORT, NULL, NULL, handler, NULL, MHD_OPTION_END);
    g_debug("WebManager started");
}

void WebManager::stop() {
    g_debug("WebManager is stopping");
    MHD_stop_daemon(daemon);
    g_debug("WebManager stopped");
}

WebManager::~WebManager() {
    stop();
}