#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <xine.h>
#include <xine/xineutils.h>

typedef enum
{
	AUDIO_CHANNEL_STATE_BOTH = 0,
	AUDIO_CHANNEL_STATE_LEFT = 1,
	AUDIO_CHANNEL_STATE_RIGHT = 2
} AudioChannelState;

static xine_t*				xine;
static xine_stream_t*		stream;
static xine_video_port_t*	video_port;
static xine_audio_port_t*	audio_port;
static xine_event_queue_t*	event_queue;

static Display*				display;
static int					screen;
static Window				window;
static double				pixel_aspect;
static int					running = 0;
static int					width = 320, height = 200;
static AudioChannelState	audio_channel_state = AUDIO_CHANNEL_STATE_BOTH;

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

int set_audio_channel_state(AudioChannelState state)
{
	static xine_post_t* plugin = NULL;
	
	if (audio_channel_state != state)
	{
		if (state == AUDIO_CHANNEL_STATE_BOTH)
		{
			if (plugin != NULL)
			{
				printf("me-tv-xine: Disabling dual language");
				xine_post_wire_audio_port (xine_get_audio_source (stream), audio_port);
			}
		}
		else
		{
			switch (state)
			{
			case AUDIO_CHANNEL_STATE_LEFT: printf("me-tv-xine: Enabling left channel\n");  break;
			case AUDIO_CHANNEL_STATE_RIGHT: printf("me-tv-xine: Enabling right channel\n");  break;
			default: break;
			}
			
			if (plugin == NULL)
			{
				printf("me-tv-xine: Creating upmix_mono plugin\n");
				xine_post_wire_audio_port(xine_get_audio_source (stream), audio_port);

				plugin = xine_post_init (xine, "upmix_mono", 0, &audio_port, &video_port);
				if (plugin == NULL)
				{
					fprintf(stderr, "me-tv-xine: Failed to create upmix_mono plugin\n");
					return -1;
				}
				
				printf("me-tv-xine: upmix_mono plugin created\n");
			}
			else
			{
				printf("me-tv-xine: upmix_mono plugin already created, using existing\n");
			}
			
			xine_post_out_t* plugin_output = xine_post_output (plugin, "audio out")
				? : xine_post_output (plugin, "audio")
				? : xine_post_output (plugin, xine_post_list_outputs (plugin)[0]);
			if (plugin_output == NULL)
			{
				fprintf(stderr, "me-tv-xine: Failed to get xine plugin output for upmix_mono");
				return -1;
			}
			
			xine_post_in_t* plugin_input = xine_post_input (plugin, "audio")
				? : xine_post_input (plugin, "audio in")
				? : xine_post_input (plugin, xine_post_list_inputs (plugin)[0]);
			
			if (plugin_input == NULL)
			{
				fprintf(stderr, "me-tv-xine: Failed to get xine plugin input for upmix_mono\n");
				return -1;
			}

			xine_post_wire (xine_get_audio_source (stream), plugin_input);
			xine_post_wire_audio_port (plugin_output, audio_port);
			
			printf("me-tv-xine: upmix_mono plugin wired\n");
			int parameter = -1;
			switch (state)
			{
			case AUDIO_CHANNEL_STATE_LEFT: parameter = 0; break;
			case AUDIO_CHANNEL_STATE_RIGHT: parameter = 1; break;
			default: break;
			}

			printf("me-tv-xine: Setting channel on upmix_mono plugin to %d\n", parameter);

			const xine_post_in_t *in = xine_post_input (plugin, "parameters");
			const xine_post_api_t* api = (const xine_post_api_t*)in->data;
			const xine_post_api_descr_t* param_desc = api->get_param_descr();
			
			if (param_desc->struct_size != 4)
			{
				fprintf(stderr, "me-tv-xine: ASSERT: parameter size != 4\n");
				return -1;
			}

			api->set_parameters (plugin, (void*)&parameter);
		}
		
		audio_channel_state = state;
	}

	return 0;
}

void set_mute_state(bool mute)
{
	xine_set_param(stream, XINE_PARAM_AUDIO_AMP_LEVEL, mute ? 0 : 100);
}
  
void set_deinterlacer_state(bool deinterlace)
{
	xine_set_param(stream, XINE_PARAM_VO_DEINTERLACE, deinterlace);
}

void set_audio_stream(int channel)
{
	xine_set_param(stream, XINE_PARAM_AUDIO_CHANNEL_LOGICAL, channel);
}

void set_subtitle_stream(int channel)
{
	xine_set_param(stream, XINE_PARAM_SPU_CHANNEL, channel);
}

int main(int argc, char **argv)
{
	char			configfile[2048];
	x11_visual_t	vis;
	double			res_h, res_v;
	char			*mrl = NULL;
	const char*		video_driver = "auto";
	const char*		audio_driver = "auto";

	if (argc != 7)
	{
		fprintf(stderr, "me-tv-xine: Invalid number of parameters\n");
		return -1;
	}
	
	mrl = argv[1];
	window = atoi(argv[2]);
	if (strlen(argv[3]) > 0)
	{
		video_driver = argv[3];
	}
	if (strlen(argv[4]) > 0)
	{
		audio_driver = argv[4];
	}

	if (!XInitThreads())
	{
		fprintf(stderr, "me-tv-xine: XInitThreads() failed\n");
		return -1;
	}

	xine = xine_new();
	sprintf(configfile, "%s%s", xine_get_homedir(), "/.me-tv/xine.config");
	xine_config_load(xine, configfile);
	xine_init(xine);

	if ((display = XOpenDisplay(getenv("DISPLAY"))) == NULL)
	{
		fprintf(stderr, "me-tv-xine: XOpenDisplay() failed.\n");
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

	if ((video_port = xine_open_video_driver(xine, video_driver, XINE_VISUAL_TYPE_X11, (void *) &vis)) == NULL)
	{
		fprintf(stderr, "me-tv-xine: Failed to initialise video driver '%s'\n", video_driver);
		return -1;
	}

	audio_port	= xine_open_audio_driver(xine , audio_driver, NULL);
	stream		= xine_stream_new(xine, audio_port, video_port);
	event_queue	= xine_event_new_queue(stream);
	xine_event_create_listener_thread(event_queue, event_listener, NULL);

	xine_port_send_gui_data(video_port, XINE_GUI_SEND_DRAWABLE_CHANGED, (void *) window);
	xine_port_send_gui_data(video_port, XINE_GUI_SEND_VIDEOWIN_VISIBLE, (void *) 1);

	if (video_port != NULL && strcmp(argv[5], "tvtime") == 0)
	{
		xine_post_wire_video_port(xine_get_video_source(stream), video_port);

		xine_post_t* plugin = xine_post_init(xine, "tvtime", 0, &audio_port, &video_port);
		if (plugin == NULL)
		{
			fprintf(stderr, "me-tv-xine: Failed to create tvtime plugin\n");
			return -1;
		}

		xine_post_out_t* plugin_output = xine_post_output (plugin, "video out")
				? : xine_post_output (plugin, "video")
				? : xine_post_output (plugin, xine_post_list_outputs (plugin)[0]);
		if (plugin_output == NULL)
		{
			fprintf(stderr, "me-tv-xine: Failed to get xine plugin output for deinterlacing\n");
			return -1;
		}

		xine_post_in_t* plugin_input = xine_post_input (plugin, "video")
				? : xine_post_input (plugin, "video in")
				? : xine_post_input (plugin, xine_post_list_inputs (plugin)[0]);

		if (plugin_input == NULL)
		{
			fprintf(stderr, "me-tv-xine: Failed to get xine plugin input for deinterlacing\n");
			return -1;
		}

		xine_post_wire(xine_get_video_source (stream), plugin_input);
		xine_post_wire_video_port(plugin_output, video_port);

		set_deinterlacer_state(true);
	}
	else if (strcmp(argv[5], "standard") == 0)
	{
		set_deinterlacer_state(true);
	}
	else
	{
		set_deinterlacer_state(false);
	}

	set_mute_state(strcmp(argv[6], "true") == 0);
	
	if ((!xine_open(stream, mrl)) || (!xine_play(stream, 0, 0)))
	{
		fprintf(stderr, "me-tv-xine: Failed to open mrl '%s'\n", mrl);
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
					XConfigureEvent* configure_event = (XConfigureEvent*)(void*)&xevent;
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
					XKeyEvent* key_event = (XKeyEvent*)(void*)&xevent;
					int keysym = XKeycodeToKeysym(display, key_event->keycode, 0);
					switch(keysym)
					{
					case XK_m:
						set_mute_state(key_event->state != XK_Control_L);
						break;
							
					case XK_a:
						if (key_event->state == (XK_Control_L & XK_Control_R))
						{
							set_audio_channel_state(AUDIO_CHANNEL_STATE_BOTH);
						}
						else if (key_event->state == XK_Control_L)
						{
							set_audio_channel_state(AUDIO_CHANNEL_STATE_LEFT);
						}
						else if (key_event->state == XK_Control_R)
						{
							set_audio_channel_state(AUDIO_CHANNEL_STATE_RIGHT);
						}
							
						break;

					case XK_space:
						if (key_event->state == XK_Shift_L)
						{
							xine_set_param(stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE);
						}
						else
						{
							xine_set_param(stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
						}
						break;
							
					default:
						if (keysym >= XK_1 || keysym <= XK_9)
						{
							if (key_event->state == XK_Control_L)
							{
								set_audio_stream(keysym - XK_1);
							}
							else if (key_event->state == XK_Control_R)
							{
								set_subtitle_stream(keysym - XK_1);
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
	printf("me-tv-xine: Xine engine terminating normally\n");

	return 0;
}
