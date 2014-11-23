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
#include <map>
#include "web_manager.h"
#include "web_controller.h"

int WebManager::handler(void * cls, struct MHD_Connection * connection, const char * url, const char * method, const char * version, const char * upload_data, size_t * upload_data_size, void ** ptr) {
  WebRequest * request;
  if (*ptr == NULL)
  {
      *ptr = (void*) new WebRequest(connection, url, method);
      return MHD_YES;
  }
  else if (*upload_data_size > 0)
  {
      request = (WebRequest *) *ptr;
      request->post_process(upload_data, *upload_data_size);
      *upload_data_size = 0;
      return MHD_YES;
  }
  else 
  {
    WebController controller;
    request = (WebRequest *) *ptr;
    *ptr = NULL;
    controller.dispatch(*request);
    
    struct MHD_Response * response = MHD_create_response_from_data(request->get_content_length(), (void*) request->get_content(), MHD_YES, MHD_NO);
    MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    MHD_add_response_header (response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*");
    int ret = MHD_queue_response(connection, request->code, response);
    MHD_destroy_response(response);
    delete request;
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
