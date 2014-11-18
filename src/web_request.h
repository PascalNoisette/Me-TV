/*
 * Copyright © 2014 Pascal Noisette
 * 
 * File:   web_request.h
 * Author: Pascal Noisette
 * Created: November 15, 2014, 1:40 PM
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
#ifndef    __WEB_REQUEST_H__
#define    __WEB_REQUEST_H__

#include <microhttpd.h>
#include <glibmm.h>

class WebRequest {
    private:
        Glib::ustring url;
        Glib::ustring method;
        
        struct MHD_Connection * connection;
        struct MHD_PostProcessor * postprocessor;
        std::map<Glib::ustring, Glib::ustring> params;
        static int iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key, const char *filename, const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size);
    public:
        WebRequest(struct MHD_Connection * connection, const char * url, const char * method);
        int post_process(const char *post_data, size_t post_data_len);
        int code;
        Glib::ustring content;
        char * get_content();
        size_t get_content_length();
        bool is(const char * method);
        bool match(const char * url);
        void addParam(Glib::ustring, Glib::ustring);
};
#endif

