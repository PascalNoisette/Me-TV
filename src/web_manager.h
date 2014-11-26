/*
 * Copyright © 2014 Pascal Noisette
 * 
 * File:   web_manager.h
 * Author: Pascal Noisette
 * Created: November 11, 2014, 12:32 PM
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
#ifndef    __WEB_MANAGER_H__
#define    __WEB_MANAGER_H__

#include <microhttpd.h>

class WebManager {
    public:
        ~WebManager();
        void start();
        void stop();
    private:
        struct MHD_Daemon * daemon = NULL;
        static int handler(void * cls, struct MHD_Connection * connection, const char * url, const char * method, const char * version, const char * upload_data, size_t * upload_data_size, void ** ptr);
};

#endif