/**
 * I use this file to detach usbhid kernel driver from the device if the kernel
 * choosed the wrong driver.
 */
#include <sys/types.h>
#include <linux/limits.h>
#include <stdio.h>
#include <errno.h>
#include <usb.h>

typedef struct usb_devices_list_s
{
  unsigned short	ids[2];
} usb_devices_list_t;


#define VID	0
#define PID	1

usb_devices_list_t	to_detach_list[] =
  {
    {0x202E, 0x0001},	/* Driver mode firmware 2.0 */
    {0x202E, 0x0002},	/* Mouse mode firmware 2.0 */
    {0x202E, 0x0003},	/* Dual control mode firmware 2.0 */
    {0x202E, 0x0004},	/* Digitizer mode firmware 2.0 */
    {0x202E, 0x0005},	/* Dual touch mode firmware 3.0 */
    {0x202E, 0x0006},	/* 4 sensors mode firmware 3.0 */
    {0x0556, 0x3556},	/* Mouse mode firmware 1.0 */
    {0x0592, 0x6956},	/* Driver mode firmware 1.0 */
    {0, 0}
  };

int				detach(struct usb_device*	dev)
{
  struct usb_dev_handle*	dh = NULL;

  if ((dh = usb_open(dev)) == NULL)
    return (0);

  if (usb_detach_kernel_driver_np(dh, 0) != 0)
    {
      perror("detach");
      usb_close(dh);
      return (0);
    }

  usb_close(dh);

  return (1);
}

int			is_to_be_detached(struct usb_device*	dev)
{
  unsigned int		i = 0;

  for (i = 0; to_detach_list[i].ids[VID] != 0; ++i)
    if (dev->descriptor.idVendor == to_detach_list[i].ids[VID] &&
	dev->descriptor.idProduct == to_detach_list[i].ids[PID])
      return (1);

  return (0);
}

int			main(void)
{
  struct usb_bus*	bus = NULL;
  struct usb_device*	dev = NULL;
  unsigned short	nb_detached = 0;

  usb_init();
  usb_find_busses();
  usb_find_devices();

  for (bus = usb_busses; bus; bus = bus->next)
    for (dev = bus->devices; dev; dev = dev->next)
      if (is_to_be_detached(dev))
	{
	  if (detach(dev))
	    ++nb_detached;
	}

  printf("detach: %d device(s) detached.\n", nb_detached);

  return (0);
}
