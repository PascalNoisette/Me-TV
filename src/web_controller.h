/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright © 2014 Pascal Noisette
 *
 *  Author: Pascal Noisette
 *  Created: November 15, 2014, 10:03 AM
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

#ifndef    __WEB_CONTROLLER_H__
#define    __WEB_CONTROLLER_H__

#include <microhttpd.h>
#include "web_request.h"

class WebController {
private:
	void sample_action(WebRequest & request);
	void get_channels_action(WebRequest & request);
	void get_recordings_action(WebRequest & request);
	void post_recordings_action(WebRequest & request);
	void delete_recording_action(WebRequest & request);
	void echo_action(WebRequest & request);
        void translate_action(WebRequest & request);
	void www_action(WebRequest & request);
	Glib::ustring secure_filename(Glib::ustring file);
public:
	void dispatch(WebRequest & request);
};
#endif
