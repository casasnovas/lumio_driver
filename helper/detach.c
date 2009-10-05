/**
 * I use this file to detach usbhid from the device if the kernel choosed the
 * wrong driver. 
 */

#include <sys/types.h>
#include <linux/limits.h>
#include <stdio.h>
#include <errno.h>
#include <usb.h>

#define VENDOR_ID1 0x0556
#define DEVICE_ID1 0x3556
#define VENDOR_ID2 0x0592
#define DEVICE_ID2 0x6956

int	main(void)
{
  struct usb_dev_handle *dh = NULL;
  struct usb_bus *bus = NULL;
  struct usb_device *dev = NULL;

  usb_init();
  usb_find_busses();
  usb_find_devices();

  for (bus = usb_busses; bus; bus = bus->next)
    for (dev = bus->devices; dev; dev = dev->next)
      if ((dev->descriptor.idVendor == VENDOR_ID1	&&
	   dev->descriptor.idProduct == DEVICE_ID1)	||
	  (dev->descriptor.idVendor == VENDOR_ID2	&&
	   dev->descriptor.idProduct == DEVICE_ID2))
	goto found;

  printf("device not found.\n");
  return (1);

found:
  if ((dh = usb_open(dev)) == NULL)
    return (2);

  if (usb_detach_kernel_driver_np(dh, 0) != 0)
  {
    perror("detach");
    return (3);
  }

  return (0);
}
