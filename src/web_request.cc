/*
 * Copyright © 2014 Pascal Noisette
 * 
 * File:   web_request.cc
 * Author: Pascal Noisette
 * Created: November 15, 2014, 1:41 PM
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

#include <cstring>
#include <glibmm.h>
#include "web_request.h"

WebRequest::WebRequest(struct MHD_Connection * connection, const char * url, const char * method) 
{
      this->connection = connection;
      this->url = url;
      this->method = method;
      if (is(MHD_HTTP_METHOD_POST)) {
        this->postprocessor = MHD_create_post_processor(connection, 512, iterate_post, (void *) this);
      }
       MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, iterate_get, (void *) this);
}

int WebRequest::post_process(const char *post_data, size_t post_data_len)
{
    if (is(MHD_HTTP_METHOD_POST)) {
        return MHD_post_process (this->postprocessor, post_data, post_data_len);
    }
    return MHD_YES;
}
int WebRequest::iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key, const char *filename, const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size)
{
  WebRequest * request = (WebRequest *) coninfo_cls;
  request->addParam(key, data);
  return MHD_YES;
}
int WebRequest::iterate_get (void *coninfo_cls, enum MHD_ValueKind kind, const char *key, const char *data)
{
    WebRequest * request = (WebRequest *) coninfo_cls;
    request->addParam(key, data);
    return MHD_YES;
}

char * WebRequest::get_content() 
{
    return strdup(content.c_str());
}

size_t WebRequest::get_content_length() 
{
    return content.length();
}

bool WebRequest::is(const char * expected_method) 
{
    return method == expected_method;
}

bool WebRequest::match(const char * expected_url) 
{
    return url.compare(expected_url) == 0;
}

void WebRequest::addParam(Glib::ustring key, Glib::ustring value) 
{
    this->params[key] = value;
}

int WebRequest::sendResponse()
{
    struct MHD_Response * response = MHD_create_response_from_data(get_content_length(), (void*) get_content(), MHD_YES, MHD_NO);
    if (content_type != "") {
        MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, content_type.c_str());
    }
    MHD_add_response_header (response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*");
    MHD_add_response_header (response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    MHD_add_response_header (response, "Access-Control-Allow-Headers", "content-type");
    int ret = MHD_queue_response(connection, code, response);
    MHD_destroy_response(response);
    return ret;
}