/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright © 2014 Pascal Noisette
 *
 *  Author: Pascal Noisette
 *  Created: November 11, 2014, 12:32 PM
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

#ifndef    __WEB_MANAGER_H__
#define    __WEB_MANAGER_H__

#include <microhttpd.h>

class WebManager {
public:
	~WebManager();
	void start();
	void stop();
private:
	MHD_Daemon * daemon = NULL;
	static int handler(void * cls, MHD_Connection * connection, char const * url, char const * method, char const * version, char const * upload_data, size_t * upload_data_size, void ** ptr);
};

#endif
