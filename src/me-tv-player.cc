#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdlib.h>
#include <string>
#include <list>
#include <vlc/vlc.h>

#define DEFAULT_VOLUME	130

libvlc_instance_t*		instance = NULL;
libvlc_media_player_t*	media_player = NULL;
libvlc_exception_t		exception;
int						volume = DEFAULT_VOLUME;

typedef std::list<std::string> StringList;

void check_exception()
{
	if (libvlc_exception_raised(&exception))
	{
		fprintf(stderr, "me-tv-vlc: %s\n", libvlc_exception_get_message(&exception));
		exit(0);
	}
}

typedef enum
{
	AUDIO_CHANNEL_STATE_BOTH = 0,
	AUDIO_CHANNEL_STATE_LEFT = 1,
	AUDIO_CHANNEL_STATE_RIGHT = 2
} AudioChannelState;

static Display*				display;
static int					screen;
static Window				window;
static double				pixel_aspect;
static int					running = 0;
static int					width = 320, height = 200;
static AudioChannelState	audio_channel_state = AUDIO_CHANNEL_STATE_BOTH;

#define INPUT_MOTION (ExposureMask | KeyPressMask | StructureNotifyMask | PropertyChangeMask)

int set_audio_channel_state(AudioChannelState state)
{
	return 0;
}

void set_mute_state(bool mute)
{
	libvlc_audio_set_mute(instance, mute, &exception);
	check_exception();
}
  
void set_deinterlacer_state(bool deinterlace)
{
}

void set_audio_stream(int channel)
{
}

void set_subtitle_stream(int channel)
{
}

int main(int argc, char **argv)
{
	char			configfile[2048];
	double			res_h, res_v;
	char			*mrl = NULL;
	const char*		video_driver = "auto";
	const char*		audio_driver = "auto";

	if (argc != 7)
	{
		fprintf(stderr, "Invalid number of parameters\n");
		return -1;
	}

	if (strncmp(argv[1], "fifo://", 7) == 0)
	{
		mrl = argv[1] + 7;
	}
	else
	{
		mrl = argv[1];
	}

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
		fprintf(stderr, "XInitThreads() failed\n");
		return -1;
	}

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

	pixel_aspect          = res_v / res_h;

	if (fabs(pixel_aspect - 1.0) < 0.01)
	{
		pixel_aspect = 1.0;
	}

	const char * const vlc_argv[] = { 
		"-I", "dummy", 
		"--ignore-config",
		"--vout-event=3",
		"--verbose=0",
		"--no-osd"
	};

	libvlc_exception_init (&exception);
	instance = libvlc_new (sizeof(vlc_argv) / sizeof(vlc_argv[0]), vlc_argv, &exception);
	check_exception();

	media_player = libvlc_media_player_new(instance, &exception);
	check_exception();

	libvlc_media_player_set_xwindow(media_player, window, &exception);
	check_exception();

	libvlc_audio_set_volume(instance, volume, &exception);
	check_exception();

	libvlc_media_t* media = libvlc_media_new(instance, mrl, &exception);
	check_exception();

	window = atoi(argv[2]);
		
	std::string vout = ":vout="; vout += argv[3];
	std::string aout = ":aout="; aout += argv[4];
	
	StringList options;
	options.push_back(":ignore-config=1");
	options.push_back(":osd=0");
	options.push_back(":file-caching=2000");
	options.push_back(vout);
	options.push_back(aout);
	options.push_back(":skip-frames=1");
	options.push_back(":drop-late-frames=1");

	options.push_back(":video-filter=deinterlace");
	options.push_back(":vout-filter=deinterlace");
	options.push_back(":deinterlace-mode=bob"); // discard,blend,mean,bob,linear,x

	options.push_back(":postproc-q=6");

	for (StringList::iterator iterator = options.begin(); iterator != options.end(); iterator++)
	{
		std::string process_option = *iterator;
		if (!process_option.empty())
		{
			libvlc_media_add_option(media, process_option.c_str(), &exception);
			check_exception();
		}
	}

	libvlc_media_player_set_media(media_player, media, &exception);
	check_exception();

	libvlc_media_release(media);

	libvlc_media_player_play(media_player, &exception);
	check_exception();

	set_mute_state(strcmp(argv[6], "true") == 0);
	
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
			usleep(1000);
			continue;
		}

		switch(xevent.type)
		{
			case Expose:
				if (xevent.xexpose.count != 0)
				{
					break;
				}
				// Send expose
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
					switch(key_event->keycode)
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
							
					default:
						if (key_event->keycode >= XK_0 && key_event->keycode <= XK_9)
						{
							if (key_event->state == XK_Control_L)
							{
								set_audio_stream(key_event->keycode - XK_0);
							}
							else if (key_event->state == XK_Control_R)
							{
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

	libvlc_media_player_release(media_player);
	media_player = NULL;
	
	libvlc_release(instance);
	instance = NULL;

	XCloseDisplay (display);

	return 0;
}
