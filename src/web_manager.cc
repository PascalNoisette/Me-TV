/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright © 2014 Pascal Noisette
 *
 *  Author: Pascal Noisette
 *  Created: November 11, 2014, 12:47 PM
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
#include <map>
#include "application.h"
#include "web_manager.h"
#include "web_controller.h"

int WebManager::handler(void * cls, MHD_Connection * connection, char const * url, char const * method, char const * version, char const * upload_data, size_t * upload_data_size, void ** ptr) {
  WebRequest * request;
  if (*ptr == NULL) {
		*ptr = (void*)new WebRequest(connection, url, method);
		return MHD_YES;
  }
  else if (configuration_manager.get_boolean_value("enable_authentification") && ((WebRequest *)*ptr)->authenticate()) {
		request = (WebRequest *)*ptr;
		*ptr = NULL;
		int ret = request->fail_authenticate();
		delete request;
		return ret;
  }
  else if (*upload_data_size > 0) {
		request = (WebRequest *)*ptr;
		request->post_process(upload_data, *upload_data_size);
		*upload_data_size = 0;
		return MHD_YES;
  }
  else {
    WebController controller;
    request = (WebRequest *)*ptr;
    *ptr = NULL;
    controller.dispatch(*request);
    int ret = request->send_response();
    delete request;
    return ret;
  }
}

void WebManager::start() {
	if (configuration_manager.get_boolean_value("enable_webinterface")) {
		g_debug("WebManager starts on port %d", configuration_manager.get_int_value("listen_port_webinterface"));
		daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, configuration_manager.get_int_value("listen_port_webinterface") , NULL, NULL, handler, NULL, MHD_OPTION_END);
		g_debug("WebManager started");
	}
}

void WebManager::stop() {
	if (daemon != NULL) {
		g_debug("WebManager is stopping");
		MHD_stop_daemon(daemon);
		daemon = NULL;
		g_debug("WebManager stopped");
	}
}

WebManager::~WebManager() {
	stop();
}
