/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright © 2014 Pascal Noisette
 *
 *  Author: Pascal Noisette
 *  Created: November 15, 2014, 1:40 PM
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

#ifndef    __WEB_REQUEST_H__
#define    __WEB_REQUEST_H__

#include <microhttpd.h>
#include <glibmm.h>

class WebRequest {
private:
	MHD_Connection * connection;
	MHD_PostProcessor * postprocessor;
	static int iterate_post (void *coninfo_cls, MHD_ValueKind kind, char const * key, char const * filename, char const * content_type, char const * transfer_encoding, char const * data, uint64_t off, size_t size);
	static int iterate_get (void * coninfo_cls, MHD_ValueKind kind, char const * key, char const * data);
public:
	int code;
	Glib::ustring content;
	Glib::ustring download_file;
	std::map<Glib::ustring, Glib::ustring> params;
	std::map<char const *, Glib::ustring> headers;
	Glib::ustring url;
	Glib::ustring method;
	WebRequest(MHD_Connection * connection, char const * url, char const * method);
	int post_process(char const * post_data, size_t post_data_len);
	MHD_Response * get_content();
	MHD_Response * get_download_file();
	bool is(char const * method);
	bool match(char const * url);
	int send_response();
	bool authenticate();
	int fail_authenticate();
};

#endif
