/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright (C) 2011 Michael Lamothe
 *  Copyright © 2014  Russel Winder
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

#include <iostream>
#include <string>
#include <cmath>

#include <giomm.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <xine.h>
#include <xine/xineutils.h>

typedef enum {
  AUDIO_CHANNEL_STATE_BOTH = 0,
  AUDIO_CHANNEL_STATE_LEFT = 1,
  AUDIO_CHANNEL_STATE_RIGHT = 2
} AudioChannelState;

static xine_t* xine;
static xine_stream_t* stream;
static xine_video_port_t* video_port;
static xine_audio_port_t* audio_port;
static xine_event_queue_t* event_queue;

static Display* display;
static int screen;
static Window window;
static double pixel_aspect;
static bool running = false;
static int width = 320, height = 200;
static AudioChannelState audio_channel_state = AUDIO_CHANNEL_STATE_BOTH;

#define INPUT_MOTION (ExposureMask | KeyPressMask | StructureNotifyMask | PropertyChangeMask)

static void dest_size_cb(void *data, int video_width, int video_height,
    double video_pixel_aspect, int *dest_width, int *dest_height,
    double *dest_pixel_aspect) {
  *dest_width = width;
  *dest_height = height;
  *dest_pixel_aspect = pixel_aspect;
}

static void frame_output_cb(void *data, int video_width, int video_height,
    double video_pixel_aspect, int *dest_x, int *dest_y, int *dest_width,
    int *dest_height, double *dest_pixel_aspect, int *win_x, int *win_y) {
  *dest_x = 0;
  *dest_y = 0;
  *win_x = 0;
  *win_y = 0;
  *dest_width = width;
  *dest_height = height;
  *dest_pixel_aspect = pixel_aspect;
}

static void event_listener(void *user_data, const xine_event_t *event) {
  switch (event->type) {
  case XINE_EVENT_UI_PLAYBACK_FINISHED:
    running = false;
    break;
  default:
    break;
  }
}

int set_audio_channel_state(AudioChannelState state) {
  static xine_post_t* plugin = NULL;
  if (audio_channel_state != state) {
    if (state == AUDIO_CHANNEL_STATE_BOTH) {
      if (plugin != NULL) {
        std::cout << "me-tv-player (xine): Disabling dual language" << std::endl;
        xine_post_wire_audio_port(xine_get_audio_source(stream), audio_port);
      }
    } else {
      switch (state) {
      case AUDIO_CHANNEL_STATE_LEFT:
        std::cout << "me-tv-player (xine): Enabling left channel" << std::endl;
        break;
      case AUDIO_CHANNEL_STATE_RIGHT:
        std::cout << "me-tv-player (xine): Enabling right channel" << std::endl;
        break;
      default:
        break;
      }
      if (plugin == NULL) {
        std::cout << "me-tv-player (xine): Creating upmix_mono plugin" << std::endl;
        xine_post_wire_audio_port(xine_get_audio_source(stream), audio_port);
        plugin = xine_post_init(xine, "upmix_mono", 0, &audio_port, &video_port);
        if (plugin == NULL) {
          std::cerr << "me-tv-player (xine): Failed to create upmix_mono plugin" << std::endl;
          return -1;
        }
        std::cout << "me-tv-player (xine): upmix_mono plugin created" << std::endl;
      } else {
        std::cout << "me-tv-player (xine): upmix_mono plugin already created, using existing" << std::endl;
      }
      xine_post_out_t* plugin_output =
          xine_post_output(plugin, "audio out") ? :
              xine_post_output(plugin, "audio") ? :
                  xine_post_output(plugin, xine_post_list_outputs(plugin)[0]);
      if (plugin_output == NULL) {
        std::cerr << "me-tv-player (xine): Failed to get xine plugin output for upmix_mono" << std::endl;
        return -1;
      }
      xine_post_in_t* plugin_input =
          xine_post_input(plugin, "audio") ? :
              xine_post_input(plugin, "audio in") ? :
                  xine_post_input(plugin, xine_post_list_inputs(plugin)[0]);
      if (plugin_input == NULL) {
        std::cerr << "me-tv-player (xine): Failed to get xine plugin input for upmix_mono" << std::endl;
        return -1;
      }
      xine_post_wire(xine_get_audio_source(stream), plugin_input);
      xine_post_wire_audio_port(plugin_output, audio_port);
      std::cout << "me-tv-player (xine): upmix_mono plugin wired" << std::endl;
      int parameter = -1;
      switch (state) {
      case AUDIO_CHANNEL_STATE_LEFT:
        parameter = 0;
        break;
      case AUDIO_CHANNEL_STATE_RIGHT:
        parameter = 1;
        break;
      default:
        break;
      }
      std::cout << "me-tv-player (xine): Setting channel on upmix_mono plugin to " << parameter << std::endl;
      const xine_post_in_t* in = xine_post_input(plugin, "parameters");
      const xine_post_api_t* api = (const xine_post_api_t*) in->data;
      const xine_post_api_descr_t* param_desc = api->get_param_descr();
      if (param_desc->struct_size != 4) {
        std::cerr << "me-tv-player (xine): ASSERT: parameter size != 4" << std::endl;
        return -1;
      }
      api->set_parameters(plugin, (void*) &parameter);
    }
    audio_channel_state = state;
  }
  return 0;
}

void set_mute_state(bool mute) {
  std::cout << "me-tv-player (xine): Setting mute state to " << mute << std::endl;
  xine_set_param(stream, XINE_PARAM_AUDIO_MUTE, mute ? 1 : 0);
}

void set_volume_state(int percent) {
  if (percent < 0) {
    percent = 0;
  } else if (percent > 200) {
    percent = 200;
  }
  std::cout << "me-tv-player (xine): Setting volume to " << percent << std::endl;
  xine_set_param(stream, XINE_PARAM_AUDIO_AMP_LEVEL, percent);
}

int get_volume_state() {
  return xine_get_param(stream, XINE_PARAM_AUDIO_AMP_LEVEL);
}

void set_deinterlacer_state(bool deinterlace) {
  std::cout << "me-tv-player (xine): Setting deinterlacer state to " << deinterlace << std::endl;
  xine_set_param(stream, XINE_PARAM_VO_DEINTERLACE, deinterlace);
}

void set_audio_stream(int channel) {
  std::cout << "me-tv-player (xine): Setting audio stream to " << channel << std::endl;
  xine_set_param(stream, XINE_PARAM_AUDIO_CHANNEL_LOGICAL, channel);
}

void set_subtitle_stream(int channel) {
  std::cout << "me-tv-player (xine): Setting subtitle stream to " << channel << std::endl;
  xine_set_param(stream, XINE_PARAM_SPU_CHANNEL, channel);
}

void inhibit_screensaver_freedesktop(gboolean activate) {
  try {
    static Glib::VariantContainerBase screensaver_inhibit_cookie;
    auto proxy = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BusType::BUS_TYPE_SESSION,
                                               "org.freedesktop.ScreenSaver",
                                               "/org/freedesktop/ScreenSaver",
                                               "org.freedesktop.ScreenSaver");
    if (activate) {
      try {
        std::vector<Glib::VariantBase> values {
        	Glib::Variant<Glib::ustring>::create("Me TV"),
        	Glib::Variant<Glib::ustring>::create("Watching television")
        };
        auto parameters = Glib::VariantContainerBase::create_tuple(values);
        screensaver_inhibit_cookie = proxy->call_sync("Inhibit", parameters);
        std::cout << "me-tv-player (xine): Sent the Inhibit message to the screensaver service" << std::endl;
      }
      catch (Gio::DBus::Error error) {
        std::cout << "me-tv-player (xine): Could not send Inhibit message to the screensaver service.\nGot a: " << error.what() << std:: endl;
      }
    }
    else {
      try {
        proxy->call_sync("Uninhibit", screensaver_inhibit_cookie);
        std::cout << "me-tv-player (xine): Sent the Uninhibit message to the screensaver service" << std::endl;
      }
      catch (Gio::DBus::Error error) {
        std::cout << "me-tv-player (xine): Could not send Uninhibit message to the screensaver service.\nGot a: " << error.what() << std:: endl;
      }
    }
  }
  catch (Gio::Error error) {
    std::cout << "me-tv-player (xine): Could not connect to screensaver service.\nGot a: " << error.what() << std:: endl;
  }
}

void inhibit_screensaver_gnome3(gboolean activate) {
  try {
    static Glib::VariantContainerBase screensaver_inhibit_cookie;
    auto proxy = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BusType::BUS_TYPE_SESSION,
                                               "org.gnome.SessionManager",
                                               "/org/gnome/SessionManager",
                                               "org.gnome.SessionManager");
    if (activate) {
      try {
        std::vector<Glib::VariantBase> values {
        	Glib::Variant<Glib::ustring>::create("Me TV"),
            Glib::Variant<guint32>::create(0),
        	Glib::Variant<Glib::ustring>::create("Watching television"),
        	Glib::Variant<guint32>::create(8) // TODO: Find out why this number has to be 8.
        };
        auto parameters = Glib::VariantContainerBase::create_tuple(values);
        screensaver_inhibit_cookie = proxy->call_sync("Inhibit", parameters);
        std::cout << "me-tv-player (xine): Sent the Inhibit message to the session manager service" << std::endl;
      }
      catch (Gio::DBus::Error error) {
        std::cout << "me-tv-player (xine): Could not send Inhibit message to the session manager service.\nGot a: " << error.what() << std:: endl;
      }
    }
    else {
      try {
        proxy->call_sync("Uninhibit", screensaver_inhibit_cookie);
        std::cout << "me-tv-player (xine): Sent the Uninhibit message to the session manager service" << std::endl;
      }
      catch (Gio::DBus::Error error) {
        std::cout << "me-tv-player (xine): Could not send Uninhibit message to the session manager service.\nGot a: " << error.what() << std:: endl;
      }
    }
  }
  catch (Gio::Error error) {
    std::cout << "me-tv-player (xine): Could not connect to session manager service.\nGot a: " << error.what() << std:: endl;
  }
}

void inhibit_screensaver(gboolean activate) {
	inhibit_screensaver_gnome3(activate);
}

int main(int argc, char **argv) {
  x11_visual_t vis;
  double res_h, res_v;
  char *mrl = NULL;
  const char* video_driver = "auto";
  const char* audio_driver = "auto";
  if (argc != 10) {
    std::cerr << "me-tv-player (xine): Invalid number of parameters" << std::endl;
    return -1;
  }
  Gio::init();
  mrl = argv[1];
  window = atoi(argv[2]);
  if (strlen(argv[3]) > 0) {
    video_driver = argv[3];
  }
  if (strlen(argv[4]) > 0) {
    audio_driver = argv[4];
  }
  if (!XInitThreads()) {
    std::cerr << "me-tv-player (xine): XInitThreads() failed" << std::endl;
    return -1;
  }
  xine = xine_new();
  std::string const configfile {std::string(xine_get_homedir()) + "/.me-tv/xine.config"};
  xine_config_load(xine, configfile.c_str());
  xine_init(xine);
  if ((display = XOpenDisplay(getenv("DISPLAY"))) == NULL) {
    std::cerr << "me-tv-player (xine): XOpenDisplay() failed." << std::endl;
    return -1;
  }
  screen = XDefaultScreen(display);
  XLockDisplay(display);
  XSelectInput(display, window, INPUT_MOTION);
  res_h = (DisplayWidth(display, screen) * 1000 / DisplayWidthMM(display, screen));
  res_v = (DisplayHeight(display, screen) * 1000 / DisplayHeightMM(display, screen));
  XWindowAttributes attributes;
  XGetWindowAttributes(display, window, &attributes);
  width = attributes.width;
  height = attributes.height;
  XSync(display, False);
  XUnlockDisplay(display);
  vis.display = display;
  vis.screen = screen;
  vis.d = window;
  vis.dest_size_cb = dest_size_cb;
  vis.frame_output_cb = frame_output_cb;
  vis.user_data = NULL;
  pixel_aspect = res_v / res_h;
  if (fabs(pixel_aspect - 1.0) < 0.01) {
    pixel_aspect = 1.0;
  }
  if ((video_port = xine_open_video_driver(xine, video_driver, XINE_VISUAL_TYPE_X11, (void *) &vis)) == NULL) {
    std::cerr << "me-tv-player (xine): Failed to initialise video driver " << video_driver << std::endl;
    return -1;
  }
  audio_port = xine_open_audio_driver(xine, audio_driver, NULL);
  stream = xine_stream_new(xine, audio_port, video_port);
  event_queue = xine_event_new_queue(stream);
  xine_event_create_listener_thread(event_queue, event_listener, NULL);
  xine_port_send_gui_data(video_port, XINE_GUI_SEND_DRAWABLE_CHANGED, (void *) window);
  xine_port_send_gui_data(video_port, XINE_GUI_SEND_VIDEOWIN_VISIBLE, (void *) 1);
  if (video_port != NULL && strcmp(argv[5], "tvtime") == 0) {
    xine_post_wire_video_port(xine_get_video_source(stream), video_port);
    xine_post_t* plugin = xine_post_init(xine, "tvtime", 0, &audio_port, &video_port);
    if (plugin == NULL) {
      std::cerr << "me-tv-player (xine): Failed to create tvtime plugin" << std::endl;
      return -1;
    }
    xine_post_out_t* plugin_output =
        xine_post_output(plugin, "video out") ? :
            xine_post_output(plugin, "video") ? :
                xine_post_output(plugin, xine_post_list_outputs(plugin)[0]);
    if (plugin_output == NULL) {
      std::cerr << "me-tv-player (xine): Failed to get xine plugin output for deinterlacing" << std::endl;
      return -1;
    }
    xine_post_in_t* plugin_input =
        xine_post_input(plugin, "video") ? :
            xine_post_input(plugin, "video in") ? :
                xine_post_input(plugin, xine_post_list_inputs(plugin)[0]);
    if (plugin_input == NULL) {
      std::cerr << "me-tv-player (xine): Failed to get xine plugin input for deinterlacing" << std::endl;
      return -1;
    }
    xine_post_wire(xine_get_video_source(stream), plugin_input);
    xine_post_wire_video_port(plugin_output, video_port);
    set_deinterlacer_state(true);
  } else if (strcmp(argv[5], "standard") == 0) {
    set_deinterlacer_state(true);
  } else {
    set_deinterlacer_state(false);
  }
  if ((!xine_open(stream, mrl)) || (!xine_play(stream, 0, 0))) {
    std::cerr << "me-tv-player (xine): Failed to open mrl " << mrl << std::endl;
    return -1;
  }
  set_audio_stream(atoi(argv[7]));
  set_subtitle_stream(atoi(argv[8]));
  set_mute_state(strcmp(argv[6], "true") == 0);
  set_volume_state(atoi(argv[9]));
  running = true;
  inhibit_screensaver(true);
  while (running) {
    XEvent xevent;
    int got_event;
    XLockDisplay(display);
    got_event = XPending(display);
    if (got_event) {
      XNextEvent(display, &xevent);
    }
    XUnlockDisplay(display);
    if (!got_event) {
      xine_usec_sleep(20000);
      continue;
    }
    switch (xevent.type) {
    case Expose:
      if (xevent.xexpose.count != 0) {
        break;
      }
      xine_port_send_gui_data(video_port, XINE_GUI_SEND_EXPOSE_EVENT, &xevent);
      break;
    case ConfigureNotify: {
        XConfigureEvent* configure_event = (XConfigureEvent*) (void*) &xevent;
        Window tmp_win;
        width = configure_event->width;
        height = configure_event->height;
        if ((configure_event->x == 0) && (configure_event->y == 0)) {
          int xpos, ypos;
          XLockDisplay(display);
          XTranslateCoordinates(display, configure_event->window, DefaultRootWindow(configure_event->display), 0, 0, &xpos, &ypos, &tmp_win);
          XUnlockDisplay(display);
        }
      }
      break;
    case KeyPress: {
        XKeyEvent* key_event = (XKeyEvent*) (void*) &xevent;
        switch (key_event->keycode) {
        case XK_m:
          set_mute_state(key_event->state != XK_Control_L);
          break;
        case XK_a:
          if (key_event->state == (XK_Control_L & XK_Control_R)) {
            set_audio_channel_state(AUDIO_CHANNEL_STATE_BOTH);
          } else if (key_event->state == XK_Control_L) {
            set_audio_channel_state(AUDIO_CHANNEL_STATE_LEFT);
          } else if (key_event->state == XK_Control_R) {
            set_audio_channel_state(AUDIO_CHANNEL_STATE_RIGHT);
          }
          break;
        case XK_space:
          if (key_event->state == XK_Shift_L) {
            xine_set_param(stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE);
          } else {
            xine_set_param(stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
          }
          break;
        default:
          if (key_event->keycode >= XK_0 && key_event->keycode <= XK_colon) {
            if (key_event->state == (XK_Control_L & XK_Control_R)) {
              set_volume_state((key_event->keycode - XK_0) * 10);
            } else if (key_event->state == XK_Control_L) {
              set_audio_stream(key_event->keycode - XK_0);
            } else if (key_event->state == XK_Control_R) {
              set_subtitle_stream(key_event->keycode - XK_0);
            }
          }
          break;
        }
      }
      break;
    default:
      break;
    }
  }
  inhibit_screensaver(false);
  xine_close(stream);
  xine_event_dispose_queue(event_queue);
  xine_dispose(stream);
  if (audio_port) {
    xine_close_audio_driver(xine, audio_port);
  }
  if (video_port != NULL) {
    xine_close_video_driver(xine, video_port);
  }
  xine_exit(xine);
  XCloseDisplay(display);
  std::cout << "me-tv-player (xine): Xine engine terminating normally" << std::endl;
  return 0;
}
