/**
 * I used this file to take the videos of the dualtouch capabilities. What it
 * does is just drawing behind the mouse.
 */

#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>

static Window	create_win(Display *dpy)
{
    Window	win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 
					  200, 200, 0, 0, BlackPixel(dpy, 0));
    XMapWindow(dpy, win);
    XSync(dpy, False);
    return (win);
}


static int	has_xi2(Display *dpy)
{
  int		major = 2;
  int		minor = 0;
  int		rc;

  rc = XIQueryVersion(dpy, &major, &minor);
  if (rc == BadRequest) 
    {
      printf("No XI2 support. Server supports version %d.%d only.\n", major, minor);
      return (0);
    } 
  else
    {
      printf("XI2 supported. Server provides version %d.%d.\n", major, minor);
      return (1);
    }
}

static void		select_events(Display *dpy, Window win)
{
  unsigned char		mask1 = 0;;
  XIEventMask		evmasks;
  
  XISetMask(&mask1, XI_Motion);
  XISetMask(&mask1, XI_ButtonPress);
  XISetMask(&mask1, XI_ButtonRelease);

  evmasks.deviceid = XIAllDevices;
  evmasks.mask_len = sizeof(mask1);
  evmasks.mask = &mask1;

  XISelectEvents(dpy, win, &evmasks, 1);
  XFlush(dpy);
}

static GC	init_gc(Display* dpy, Window win)
{
  GC		teton = XCreateGC(dpy, win, 0, 0);

  XSetForeground(dpy, teton, WhitePixel(dpy, DefaultScreen(dpy)));
  XSetLineAttributes(dpy, teton, 3, LineSolid, CapRound, JoinRound);

  return (teton);
}

int			main(int argc, char **argv)
{
  unsigned int		x[25], y[25], up[25];
  unsigned int		counter = 0;
  Display		*dpy = NULL;
  Window		win = 0;
  XEvent		ev;
  GC			gc = 0;
  int			xi_opcode = 0;
  int			event = 0;
  int			error = 0;

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
  memset(up, 0x0, 25 * sizeof(int));

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
	  real_ev = cookie->data;
	  
	  if (cookie->evtype == XI_Motion && up[real_ev->deviceid])
	    {
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
	  else if (cookie->evtype == XI_ButtonPress)
	    {
	      XDrawLine(dpy, win, gc,
			real_ev->event_x, real_ev->event_y,
			real_ev->event_x, real_ev->event_y);
	      up[real_ev->deviceid] = 1;
	    }
	  else if (cookie->evtype == XI_ButtonRelease)
	    {
	      x[real_ev->deviceid] = 0;
	      y[real_ev->deviceid] = 0;
	      up[real_ev->deviceid] = 0;
	    }
	  XFreeEventData(dpy, &ev.xcookie);
	}
      XFlush(dpy);
      ++counter;
    }

  XFreeGC(dpy, gc);

  return 0;
}
