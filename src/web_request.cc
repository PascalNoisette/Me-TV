/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright © 2014 Pascal Noisette
 *
 *  Author: Pascal Noisette
 *  Created: November 15, 2014, 1:41 PM
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

#include <cstring>
#include <glibmm.h>
 #include <fcntl.h>
 #include <sys/types.h>
 #include <sys/stat.h>
#include "web_request.h"
#include "application.h"

WebRequest::WebRequest(MHD_Connection * connection, char const * url, char const * method) {
	this->connection = connection;
	this->url = url;
	this->method = method;
	if (is(MHD_HTTP_METHOD_POST)) {
		this->postprocessor = MHD_create_post_processor(connection, 512, iterate_post, (void *)this);
	}
	MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, iterate_get, (void *)this);
	headers[MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN]= "*";
	headers["Access-Control-Allow-Methods"]= "GET, POST, PUT, DELETE, OPTIONS";
	headers["Access-Control-Allow-Headers"]= "content-type";
}

int WebRequest::post_process(char const * post_data, size_t post_data_len) {
	if (is(MHD_HTTP_METHOD_POST)) {
		return MHD_post_process (this->postprocessor, post_data, post_data_len);
	}
	return MHD_YES;
}

int WebRequest::iterate_post (void * coninfo_cls, MHD_ValueKind kind, char const * key, char const * filename, char const * content_type, char const * transfer_encoding, char const * data, uint64_t off, size_t size) {
  WebRequest * request = (WebRequest *)coninfo_cls;
  request->params[key] = data;
  return MHD_YES;
}

int WebRequest::iterate_get (void * coninfo_cls, MHD_ValueKind kind, char const * key, char const * data) {
	WebRequest * request = (WebRequest *)coninfo_cls;
	request->params[key] = data;
	return MHD_YES;
}

MHD_Response * WebRequest::get_content() {
	return MHD_create_response_from_data(content.bytes(), (void*)strdup(content.c_str()), MHD_YES, MHD_NO);
}

bool WebRequest::is(char const * expected_method) {
	return method == expected_method;
}

bool WebRequest::match(char const * expected_url) {
	return url.compare(expected_url) == 0;
}

int WebRequest::send_response() {
	MHD_Response * response = NULL;
	if (download_file != "") {
		response = get_download_file();
	}
	if (response == NULL) {
		response = get_content();
	}
	for (std::map<const char*, Glib::ustring>::iterator iter = headers.begin(); iter != headers.end(); ++iter) {
		MHD_add_response_header (response, iter->first, iter->second.c_str());
	}
	int ret = MHD_queue_response(connection, code, response);
	MHD_destroy_response(response);
	return ret;
}

MHD_Response * WebRequest::get_download_file() {
	int fd;
	struct stat sbuf;
	if ((-1 == (fd = open(download_file.c_str(), O_RDONLY))) || (0 != fstat(fd, &sbuf))) {
		if (fd != -1) {
			(void) close(fd);
		}
		content = "404";
		code = MHD_HTTP_NOT_FOUND;
		return NULL;
	}
	return MHD_create_response_from_fd(sbuf.st_size, fd);
}

bool WebRequest::authenticate() {
	char * user = NULL;
	char * pass = NULL;
	user = MHD_basic_auth_get_username_password (connection, &pass);
	int fail = ( (user == NULL) ||
							 (0 != strcmp (user, configuration_manager.get_string_value("webinterface_username").c_str())) ||
							 (0 != strcmp (pass, configuration_manager.get_string_value("webinterface_password").c_str()) ) );
	if (user != NULL) free (user);
	if (pass != NULL) free (pass);
	return fail;
}

int WebRequest::fail_authenticate() {
	content = "401";
	code = MHD_HTTP_UNAUTHORIZED;
	MHD_Response * response = get_content();
	int ret = MHD_queue_basic_auth_fail_response (connection, PACKAGE_NAME, response);
	MHD_destroy_response (response);
	return ret;
}
