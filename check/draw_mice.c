/**
 * I used this file to take the videos of the dualtouch capabilities. What it
 * does is just drawing behind the mouse.
 */

#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>

static Window create_win(Display *dpy)
{
    Window win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 200,
            200, 0, 0, BlackPixel(dpy, 0));

    XMapWindow(dpy, win);
    XSync(dpy, False);
    return win;
}


/* Return 1 if XI2 is available, 0 otherwise */
static int has_xi2(Display *dpy)
{
    int major, minor;
    int rc;

    /* We support XI 2.0 */
    major = 2;
    minor = 0;

    rc = XIQueryVersion(dpy, &major, &minor);
    if (rc == BadRequest) {
        printf("No XI2 support. Server supports version %d.%d only.\n", major, minor);
        return 0;
    } else if (rc != Success) {
        fprintf(stderr, "Internal Error! This is a bug in Xlib.\n");
    }

    printf("XI2 supported. Server provides version %d.%d.\n", major, minor);

    return 1;
}

static void select_events(Display *dpy, Window win)
{
    XIEventMask evmasks[1];
    unsigned char mask1[(XI_LASTEVENT + 7)/8];

    memset(mask1, 0, sizeof(mask1));

    /* select for button and key events from all master devices */
    XISetMask(mask1, XI_Motion);

    evmasks[0].deviceid = XIAllDevices;
    evmasks[0].mask_len = sizeof(mask1);
    evmasks[0].mask = mask1;

    XISelectEvents(dpy, win, evmasks, 1);
    XFlush(dpy);
}

static GC init_gc(Display* dpy, Window win)
{
  GC teton = XCreateGC(dpy, win, 0, 0);

  XSetForeground(dpy, teton, WhitePixel(dpy, DefaultScreen(dpy)));
  XSetLineAttributes(dpy, teton, 3, LineSolid, CapRound, JoinRound);

  return teton;
}

int	main(int argc, char **argv)
{
  unsigned int	x[25], y[25];
  int		xi_opcode, event, error;
  Display	*dpy;
  Window	win;
  XEvent	ev;
  GC		gc;
  unsigned int	counter = 0;

  dpy = XOpenDisplay(NULL);
  if (!dpy)
    {
      fprintf(stderr, "Failed to open display.\n");
      return -1;
    }

  if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &event, &error))
    {
      printf("X Input extension not available.\n");
      return -1;
      }
  if (!has_xi2(dpy))
    return -1;

  win = create_win(dpy);
  select_events(dpy, win);
  gc = init_gc(dpy, win);

  memset(x, 0x0, 25 * sizeof(int));
  memset(y, 0x0, 25 * sizeof(int));

  while(1)
    {
      XGenericEventCookie *cookie = &ev.xcookie;
      XIDeviceEvent* real_ev;
      XNextEvent(dpy, &ev);

      if (cookie->type != GenericEvent ||
      	  cookie->extension != xi_opcode)
      	continue;

      if (XGetEventData(dpy, cookie))
	{
	  if (cookie->evtype == XI_Motion)
	    {
	      
	      real_ev = cookie->data;
	      /* printf("W x: %f, y: %f, id: %d\n", real_ev->event_x, real_ev->event_y, real_ev->deviceid); */
	      /* printf("R x: %f, y: %f, id: %d\n", real_ev->root_x, real_ev->root_y, real_ev->deviceid); */

	      if (x[real_ev->deviceid] == 0 && y[real_ev->deviceid] == 0)
	      	{
	      	  x[real_ev->deviceid] = real_ev->event_x;
	      	  y[real_ev->deviceid] = real_ev->event_y;
	      	}
	      else
	      	{
	      	  XDrawLine(dpy, win, gc,
	      		    x[real_ev->deviceid], y[real_ev->deviceid],
	      		    real_ev->event_x, real_ev->event_y);
	      	  x[real_ev->deviceid] = real_ev->event_x;
	      	  y[real_ev->deviceid] = real_ev->event_y;
	      	}
	    }
	  else
	    printf("What's this event ? :%d\n", cookie->evtype);
	  XFreeEventData(dpy, &ev.xcookie);
	}
      if (counter == 100)
	{
	  XFlush(dpy);
	  counter = 0;
	}
      ++counter;
    }

  XFreeGC(dpy, gc);

  return 0;
}
