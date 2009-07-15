#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <xine.h>
#include <xine/xineutils.h>

static xine_t              *xine;
static xine_stream_t       *stream;
static xine_video_port_t   *video_port;
static xine_audio_port_t   *audio_port;
static xine_event_queue_t  *event_queue;

static Display             *display;
static int                  screen;
static Window               window;
static double               pixel_aspect;
static int                  running = 0;
static int					width = 320, height = 200;

#define INPUT_MOTION (ExposureMask | KeyPressMask | StructureNotifyMask | PropertyChangeMask)

static void dest_size_cb(void *data, int video_width, int video_height, double video_pixel_aspect,
			 int *dest_width, int *dest_height, double *dest_pixel_aspect)
{
	*dest_width        = width;
	*dest_height       = height;
	*dest_pixel_aspect = pixel_aspect;
}

static void frame_output_cb(void *data, int video_width, int video_height,
			    double video_pixel_aspect, int *dest_x, int *dest_y,
			    int *dest_width, int *dest_height, 
			    double *dest_pixel_aspect, int *win_x, int *win_y)
{
	*dest_x            = 0;
	*dest_y            = 0;
	*win_x             = 0;
	*win_y             = 0;
	*dest_width        = width;
	*dest_height       = height;
	*dest_pixel_aspect = pixel_aspect;
}

static void event_listener(void *user_data, const xine_event_t *event)
{
	switch(event->type)
	{
		case XINE_EVENT_UI_PLAYBACK_FINISHED:
			running = 0;
			break;
		default:
			break;
	}
}
  
int main(int argc, char **argv)
{
	char			configfile[2048];
	x11_visual_t	vis;
	double			res_h, res_v;
	char			*mrl = NULL;

	if (argc != 6)
	{
		fprintf(stderr, "Invalid number of parameters\n");
		return -1;
	}
	
	mrl = argv[1];
	window = atoi(argv[2]);

	if (!XInitThreads())
	{
		fprintf(stderr, "XInitThreads() failed\n");
		return -1;
	}

	xine = xine_new();
	sprintf(configfile, "%s%s", xine_get_homedir(), "/.me-tv/xine.config");
	xine_config_load(xine, configfile);
	xine_init(xine);

	if ((display = XOpenDisplay(getenv("DISPLAY"))) == NULL)
	{
		fprintf(stderr, "XOpenDisplay() failed.\n");
		return -1;
	}

	screen = XDefaultScreen(display);

	XLockDisplay(display);
	XSelectInput (display, window, INPUT_MOTION);
	res_h = (DisplayWidth(display, screen) * 1000 / DisplayWidthMM(display, screen));
	res_v = (DisplayHeight(display, screen) * 1000 / DisplayHeightMM(display, screen));

	XWindowAttributes attributes;
	XGetWindowAttributes(display, window, &attributes);
	width = attributes.width;
	height = attributes.height;

	XSync(display, False);
	XUnlockDisplay(display);

	vis.display           = display;
	vis.screen            = screen;
	vis.d                 = window;
	vis.dest_size_cb      = dest_size_cb;
	vis.frame_output_cb   = frame_output_cb;
	vis.user_data         = NULL;
	pixel_aspect          = res_v / res_h;

	if (fabs(pixel_aspect - 1.0) < 0.01)
	{
		pixel_aspect = 1.0;
	}

	if ((video_port = xine_open_video_driver(xine, argv[3], XINE_VISUAL_TYPE_X11, (void *) &vis)) == NULL)
	{
		fprintf(stderr, "Failed to initialise video driver '%s'\n", argv[3]);
		return -1;
	}

	audio_port	= xine_open_audio_driver(xine , argv[4], NULL);
	stream		= xine_stream_new(xine, audio_port, video_port);
	event_queue	= xine_event_new_queue(stream);
	xine_event_create_listener_thread(event_queue, event_listener, NULL);

	if (video_port != NULL)
	{
		xine_post_wire_video_port(xine_get_video_source(stream), video_port);

		xine_post_t* plugin = xine_post_init(xine, "tvtime", 0, &audio_port, &video_port);
		if (plugin == NULL)
		{
			fprintf(stderr, "Failed to create tvtime plugin\n");
			return -1;
		}

		xine_post_out_t* plugin_output = xine_post_output (plugin, "video out")
				? : xine_post_output (plugin, "video")
				? : xine_post_output (plugin, xine_post_list_outputs (plugin)[0]);
		if (plugin_output == NULL)
		{
			fprintf(stderr, "Failed to get xine plugin output for deinterlacing\n");
			return -1;
		}

		xine_post_in_t* plugin_input = xine_post_input (plugin, "video")
				? : xine_post_input (plugin, "video in")
				? : xine_post_input (plugin, xine_post_list_inputs (plugin)[0]);

		if (plugin_input == NULL)
		{
			fprintf(stderr, "Failed to get xine plugin input for deinterlacing\n");
			return -1;
		}

		xine_post_wire(xine_get_video_source (stream), plugin_input);
		xine_post_wire_video_port(plugin_output, video_port);

		xine_set_param(stream, XINE_PARAM_VO_DEINTERLACE, true);
	}
	
	xine_port_send_gui_data(video_port, XINE_GUI_SEND_DRAWABLE_CHANGED, (void *) window);
	xine_port_send_gui_data(video_port, XINE_GUI_SEND_VIDEOWIN_VISIBLE, (void *) 1);

	bool mute = strcmp(argv[5], "mute") == 0;
	xine_set_param(stream, XINE_PARAM_AUDIO_AMP_LEVEL, mute ? 0 : 100);
	
	if ((!xine_open(stream, mrl)) || (!xine_play(stream, 0, 0)))
	{
		fprintf(stderr, "Failed to open mrl '%s'\n", mrl);
		return -1;
	}

	running = 1;

	while (running)
	{
		XEvent xevent;
		int got_event;

		XLockDisplay(display);
		got_event = XPending(display);
		if (got_event)
		{
			XNextEvent(display, &xevent);
		}
		XUnlockDisplay(display);

		if (!got_event)
		{
			xine_usec_sleep(20000);
			continue;
		}

		switch(xevent.type)
		{
			case Expose:
				if (xevent.xexpose.count != 0)
				{
					break;
				}
				xine_port_send_gui_data(video_port, XINE_GUI_SEND_EXPOSE_EVENT, &xevent);
				break;

			case ConfigureNotify:
				{
					XConfigureEvent* configure_event = (XConfigureEvent*)&xevent;
					Window tmp_win;

					width  = configure_event->width;
					height = configure_event->height;

					if ((configure_event->x == 0) && (configure_event->y == 0))
					{
						int xpos, ypos;
						XLockDisplay(display);
						XTranslateCoordinates(display, configure_event->window,
							DefaultRootWindow(configure_event->display),
							0, 0, &xpos, &ypos, &tmp_win);
						XUnlockDisplay(display);
					}
				}
				break;

			case KeyPress:
				{
					XKeyEvent* key_event = (XKeyEvent*)&xevent;
					int keysym = XKeycodeToKeysym(display, key_event->keycode, 0);
					switch(keysym)
					{
					case XK_m:
						printf("*** AMP_LEVEL = %d ***\n", (key_event->state == XK_Shift_L) ? 100 : 0);
						xine_set_param(stream, XINE_PARAM_AUDIO_AMP_LEVEL,
							(key_event->state == XK_Shift_L) ? 100 : 0);
						break;
							
					default:
						break;
					}
				}
				break;

			default:
				break;
		}
	}
  
	xine_close(stream);
	xine_event_dispose_queue(event_queue);
	xine_dispose(stream);
	if (audio_port)
	{
		xine_close_audio_driver(xine, audio_port);
	}
	if (video_port != NULL)
	{
		xine_close_video_driver(xine, video_port);
	}
	xine_exit(xine);

	XCloseDisplay (display);
	printf("Xine engine terminating normally");

	return 0;
}
