/*
    (c) Quentin Casasnovas

    This file is part of lumio_driver.

    lumio_driver is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    lumio_driver is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with lumio_driver. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file lumio_driver.c
 * @author Quentin Casasnovas
 * @brief Contains all the driver code.
 *
 *	This file contains the core of this usb driver. Everything you need to
 * understand the protocol of the lumio touch screen should be documented here.
 */

/**
 * @mainpage 
 * @version 0.1
 * @author Quentin Casasnovas
 *
 * @verbinclude ../README
 *
 */

#include "lumio_driver_.h"

#define LUMIO_DEBUG 1

MODULE_DESCRIPTION("USB lumio multi-touchscreen driver");
MODULE_AUTHOR("Quentin Casasnovas");
MODULE_LICENSE("GPL");

static struct usb_device_id lumio_id_table[] __devinitdata =
  {
    {USB_DEVICE(USB_VID_MM, USB_PID_MM)},
    {USB_DEVICE(USB_VID_DM, USB_PID_DM)},
    {USB_DEVICE(USB_VID_LUMIO, USB_PID_DM_2_0)},
    {USB_DEVICE(USB_VID_LUMIO, USB_PID_MM_2_0)},
    {USB_DEVICE(USB_VID_LUMIO, USB_PID_DUAL_MODE_3_0)},
    {USB_DEVICE(USB_VID_LUMIO, USB_PID_4_SENSORS_3_0)},
    {},
  };

MODULE_DEVICE_TABLE(usb, lumio_id_table);

static struct usb_driver lumio_driver;

const char*	idev_name1 = "Lumio touchscreen1";
const char*	idev_name2 = "Lumio touchscreen2";

/**
 * @brief Destructor of this driver private data.
 *
 *	This function is called when the refcounter of our private data equals
 * 0, it means that it is not referenced anymore and that we could free the
 * memory with no risk. See Documentation/kref.txt for more informations on
 * reference counting in the linux kernel.
 *
 * @param refcount A pointer to the reference counter of our data.
 */
static void			lumio_delete(struct kref* refcount)
{
  struct usb_touchscreen*	data =
    container_of(refcount, struct usb_touchscreen, refcount);

  usb_set_intfdata(data->interface, NULL);

  if (data->urb_in)
    {
      usb_kill_urb(data->urb_in);
      usb_free_urb(data->urb_in);
    }
  if (data->urb_in2)
    {
      usb_kill_urb(data->urb_in2);
      usb_free_urb(data->urb_in2);
    }
  if (data->in_buffer)
    kfree(data->in_buffer);
  if (data->out_buffer)
    kfree(data->out_buffer);
  if (data->fakemouse[0].idev)
    input_unregister_device(data->fakemouse[0].idev);
  if (data->fakemouse[1].idev)
    input_unregister_device(data->fakemouse[1].idev);
  usb_put_dev(data->udev);
  kfree(data);
}

/**
 * @brief Extends features of this driver.
 *
 *	At first glance, the driver only creates two input device, emulating
 * two mice in the system. But it may be tuned to better suit your needs using
 * this function. For now it only supports setting/getting the delay in which a
 * touch should be recognized as a click (default delay is 20
 * milliseconds). IOCTL commands supported are define in lumio_driver.h.
 *
 * @param inode Used to retreive the minor for this device.
 * @param file
 * @param cmd One of the ioctl commands defined in lumio_driver.h.
 * @param arg
 *	The parameter for each comand, authorized values, depending on the cmd
 * argument are :
 * - No commands are supported for now.
 * double clic.
 * @return 0 on success, a negative number on failure.
 */
static int			lumio_ioctl(struct inode*	inode,
					 struct file*	file,
					 unsigned int	cmd,
					 unsigned long	arg)
{
  struct usb_interface*		interface;
  struct usb_touchscreen*	data;
  int				minor;

  minor = MINOR(inode->i_rdev);
  interface = usb_find_interface(&lumio_driver, minor);
  if (!interface)
    {
      printk(KERN_WARNING "lumio_driver: cannot find device for minor.\n");
      return (-ENODEV);
    }

  data = usb_get_intfdata(interface);
  if (!data)
    return (-ENODEV);

  switch (cmd)
    {
    default:
      printk(KERN_WARNING "lumio_driver: 0x%x unsupported ioctl command.\n", cmd);
      return (-EINVAL);
      break;
    }

  return (0);
}

/**
 * @brief Opens the lumio%d char device.
 *
 *	This function is called when a userland programm opens the char device
 * associated with the lumio touchscreen. A userland programm should only open
 * this device to configure the way this driver manages the touchscreen (see
 * lumio_ioctl()).
 *
 *	If you would like to receive events from the touchscreen, you
 * should open the two emulated mice this driver has created, which should be
 * /dev/input/mouseX (to find what the X is simply cat each mouse and play with
 * the touchscreen, when you see strange characters printed on you term, you
 * found the right char device to open.
 *
 * @warning Again this is not used to receive events from the touchscreen but
 * to configure the driver !
 *
 * @param inode Used to retreive the minor.
 * @param file Used to attach private data to the char device.
 * @return 0 on success, a negative number on failure.
 */
static int			lumio_open(struct inode*	inode,
					struct file*	file)
{
  struct usb_interface*		interface;
  struct usb_touchscreen*	data;
  int				minor;

  /* Minor retreiving */
  minor = MINOR(inode->i_rdev);
  interface = usb_find_interface(&lumio_driver, minor);
  if (!interface)
    {
      printk(KERN_WARNING "lumio_driver: cannot find device for minor.\n");
      return (-ENODEV);
    }

  /* Attaching our data to the usb device */
  data = usb_get_intfdata(interface);
  kref_get(&data->refcount);
  if (!data)
    return (-ENODEV);

  file->private_data = data;

  return (0);
}

/**
 * @brief Closes the fs%d char device.
 *
 *	This function is called when a userland programm closes the char device
 * fs%d used to configure this driver (see lumio_ioctl()). The only this function
 * does is releasing the private datas attached to the file descriptor when
 * opening it.
 *
 * @param inode
 * @param file
 * @return Always 0.
 */
static int			lumio_release(struct inode*	inode,
					   struct file*		file)
{
  int				ret = 0;
  struct usb_touchscreen*	data;

  data = file->private_data;

  if (data)
    kref_put(&data->refcount, lumio_delete);
  else
    ret = -ENODEV;

  file->private_data = NULL;

  return (ret);
}

/**
 * @brief
 *	This structure tells the kernel which function we register with the
 *	char device.
 */
static struct file_operations	lumio_fops =
  {
    .owner	= THIS_MODULE,
    .open	= lumio_open,
    .release	= lumio_release,
    .ioctl	= lumio_ioctl,
  };

/**
 * @brief
 *	This structure tells the kernel which char device we will use for this
 *	driver.
 */
static struct usb_class_driver	lumio_class =
  {
    .name	= "lumio%d",
    .fops	= &lumio_fops,
    .minor_base	= 0,
  };

static int		lumio_send_8_bytes(struct usb_touchscreen*	data,
					   unsigned int			hid_type)
{
  printk(KERN_INFO "lumio_driver: send_8_bytes fired!\n");
  return (usb_control_msg(data->udev,
			  usb_sndctrlpipe(data->udev, 0),
			  HID_REQ_SET_REPORT,
			  USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			  hid_type,
			  0, data->out_buffer, 8, 0));
}

static void		lumio_send_64_aknowledged(struct urb* urb)
{
  struct usb_touchscreen*	data = NULL;

  ASSERT(urb != NULL);
  ASSERT(urb->context != NULL);

  data = urb->context;
};

static int		lumio_send_64_bytes(struct usb_touchscreen*	data,
					    unsigned int		hid_type)
{
  printk(KERN_INFO "lumio_driver: send_64_bytes fired!\n");
  return (usb_submit_urb(data->urb_commander, GFP_KERNEL));
}

static int		lumio_recv_8_bytes(struct usb_touchscreen*	data,
					    unsigned int		hid_type)
{
  return (usb_control_msg(data->udev,
			  usb_rcvctrlpipe(data->udev, 0),
			  HID_REQ_GET_REPORT,
			  USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			  hid_type,
			  0, data->in_buffer, 8, 0));
}

/**
 * @brief Tells the controller to switch to driver mode.
 *
 *	The controller may be in two different states (as explained here
 * lumio_probe()). This function sets the controller to driver mode sending it a
 * control urb containing the following 8 bytes : 0x7576000000000000, and
 * then closing the interrupt pipe. The controller should have switched within
 * 2000 milliseconds.
 *
 * @param data The private data asociated with the interface.
 * @return 0 on success, a negative number in other cases.
 */
static int		lumio_switch_to_driver_mode(struct usb_touchscreen* data)
{
  int			ret = 0;

  ASSERT(data != NULL);
  ASSERT(data->out_buffer != NULL);

  /* Constructing the message */
  memset(data->out_buffer, 0x0, 8);
  data->out_buffer[0] = 0x75;
  data->out_buffer[1] = 0x76;

  SAFE_CALL(data->send_msg(data, HID_VAL_FEATURE),
	    "Unable to send control urb.\n");

  return (0);

 error:

  return (ret);
}

/**
 * @brief Asks the controller the actual configuration (single/dualtouch).
 *
 *	This function asks the controller, which should already be in driver
 * mode, if it reports single or dualtouch events. This is done by sending it
 * the following 8 bytes 0x7F9B000000000000. We then waits for its reply, which
 * will be 0x7F9B010100000000 if it reports dualtouch events, and
 * 0x7F9B010000000000 for signletouch events.
 *
 * @param data Private data associated with the device.
 * @return Return a positive number corresponding to USB_DUALTOUCH_CONFIG or
 * USB_SINGLETOUCH_CONFIG if there's no problem, a negative number if it fails
 * to ask the controller.
 */
static int	lumio_actual_conf(struct usb_touchscreen* data)
{
  int		ret = 0;

  ASSERT(data != NULL);
  ASSERT(data->out_buffer != NULL);
  ASSERT(data->in_buffer != NULL);

  /* Constructing the message */
  memset(data->out_buffer, 0x0, 8);
  data->out_buffer[0] = 0x7F;
  data->out_buffer[1] = 0x9B;

  if (data->firmware_version != LUMIO_FIRMWARE_3_0)
    {
      SAFE_CALL(data->send_msg(data, HID_REQ_SET_REPORT),
		"Unable to ask actual configuration.\n");
      
      mdelay(50);
      
      memset(data->in_buffer, 0x0, 16);
      SAFE_CALL(data->recv_msg(data, HID_REQ_GET_REPORT),
		"Unable to receive actual configuration.\n");
    }
  else
    return (USB_DUALTOUCH_CONFIG);

  if (data->in_buffer[3] == 0x01)
    return (USB_DUALTOUCH_CONFIG);
  else
    return (USB_SINGLETOUCH_CONFIG);

 error:
  return (ret);
}

/**
 * @brief Tells the controller to report dual touch events.
 *
 *	By default, in driver mode, the controller only reports single touch
 * events (see lumio_probe()). Here we tell it to notify us dual touch events
 * sending it the following 8 bytes : 0x7F9B010100000000.
 *
 *	As the controller may be asked to tell in what configuration it is (see
 * lumio_actual_conf()), it's a good habit to check that it has really changed its
 * configuration to dual touch. If that is not the case we try to set it again
 * to dual touch configuration. After that last try, if the controller is still
 * not in dual touch configuration, an error is generated.
 *
 * @param data The private data asociated with the interface.
 * @return 0 on success and a negative number if it fails.
 */
static int	lumio_switch_to_dualtouch_mode(struct usb_touchscreen* data)
{
  int		nb_try = 0;

  ASSERT(data != NULL);
  ASSERT(data->out_buffer != NULL);

  do
    {
      memset(data->out_buffer, 0x0, 64);
      data->out_buffer[0] = 0x7F;
      data->out_buffer[1] = 0x9B;
      data->out_buffer[2] = 0x01;
      data->out_buffer[3] = 0x01;
      if (data->send_msg(data, HID_REQ_SET_REPORT) != 0)
	printk(KERN_INFO "lumio_driver: Can't set to dual touch mode... Retrying.\n");

      mdelay(10);
      ++nb_try;
    } while (nb_try < 3 && lumio_actual_conf(data) == USB_SINGLETOUCH_CONFIG);

  if (nb_try == 3)
    return (-ENODEV);

  return (0);
}

/**
 * @brief Tells the controller to report single touch events.
 *
 *	This function tells the controller to report single touch events
 * sending it the following 8 bytes : 0x7F9B010000000000.
 *
 *	As the controller may be asked to tell in what configuration it is (see
 * lumio_actual_conf()), it's a good habit to check that it has really changed its
 * configuration to dual touch. If that is not the case we try to set it again
 * to dual touch configuration. After that last try, if the controller is still
 * not in dual touch configuration, an error is generated.
 *
 * @param data The private data asociated with the interface.
 * @return 0 on success and a negative number if it fails.
 */
/* static int	lumio_switch_to_singletouch_mode(struct usb_touchscreen* data) */

/**
 * @brief Discovers all endpoints the device has to offer.
 *
 *	 This functions probes all endpoints attached to the device and record
 * them in the private structure associated with the interface (see
 * ::usb_touchscreen). Endpoints are kind off pipes used to communicate with the
 * touchscreen.
 *
 * @param data The private structure associated with the interface.
 * @return 0 on success, a negative number if it fails probing endpoints.
 */
static int				lumio_find_endpoints(struct usb_touchscreen* data)
{
  unsigned int				i;
  struct usb_host_interface*		iface_desc;
  struct usb_endpoint_descriptor*	endpoint;

  ASSERT(data != NULL);

  iface_desc = data->interface->cur_altsetting;

  for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i)
    {
      endpoint = &iface_desc->endpoint[i].desc;
      if ((endpoint->bEndpointAddress & USB_DIR_IN) &&
	  IS_INT_ENDPOINT(endpoint->bmAttributes))
	{
	  data->int_in_endpoint = endpoint->bEndpointAddress;
	}
      else if (!(endpoint->bEndpointAddress & USB_DIR_IN) &&
	       IS_INT_ENDPOINT(endpoint->bmAttributes))
	{
	  data->int_out_endpoint = endpoint->bEndpointAddress;
	  printk(KERN_INFO "lumio_driver: Out interrupt endpoint adress is: %d.\n",
		 data->int_out_endpoint);
	}
    }

  return (0);
}

/**
 * @brief Starts the receiving of urbs.
 *
 *	This function is called each first time one of the two char device
 * corresponding to our fake mice is opened. That means somebody is listening
 * to events from our touchscreen and that we should start receiving interrupt
 * in urbs to get report from the touchscreen.
 *
 * @param dev The char device that is first read by a process.
 */
static int			lumio_fake_open(struct input_dev* dev)
{
  struct usb_touchscreen*	data = input_get_drvdata(dev);
  int				ret = 0;

  ASSERT(data != NULL);

  if (data->listeners == 0)
    {
      SAFE_CALL(usb_submit_urb(data->urb_in, GFP_KERNEL),
		"Unable to register int in urb.\n");
      data->listeners = 1;
    }
  else
    ++data->listeners;

  printk(KERN_INFO "lumio_driver: O nb_listeners: %d\n", data->listeners);
  return (0);

 error:
  return (ret);
}

/**
 * @brief Stops the urb if nobody is listenning.
 *
 *	This function is called by the input layer when the last process has
 * released one of the two char devices that emulates mice. As there only one
 * touchscreen for two fake mice, we actually stops receiving urb only when the two
 * char devices are not being red anymore.
 *
 * @param dev The input device that is not readed anymore.
 */
static void			lumio_fake_close(struct input_dev* dev)
{
  struct usb_touchscreen*	data = input_get_drvdata(dev);

  ASSERT(data != NULL);

  --data->listeners;
  if (data->listeners == 0)
    {
      usb_kill_urb(data->urb_in);
      usb_kill_urb(data->urb_in2);
    }
  printk(KERN_INFO "lumio_driver: nb_listeners: %d\n", data->listeners);
}

static void			lumio_which_finger(struct usb_touchscreen* data,
						   __u8*	which,
						   __u32	x,
						   __u32	y)
{
  __u32				distance_from_first_finger = 0;
  __u32				distance_from_second_finger = 0;

  ASSERT(data != NULL);
  ASSERT(which != NULL);

  distance_from_first_finger =
    abs(data->fakemouse[0].last_x - x) + abs(data->fakemouse[0].last_y - y);
  distance_from_second_finger =
    abs(data->fakemouse[1].last_x - x) + abs(data->fakemouse[1].last_y - y);

  if (distance_from_first_finger < distance_from_second_finger)
    *which = 0;
  else
    *which = 1;
}

static void			lumio_treat_event(struct usb_touchscreen*	data,
						  int				event_type)
{
  unsigned char*		event = NULL;
  __u8				which = 0;
  __u32				x = 0;
  __u32				y = 0;

  ASSERT(data != NULL);
  ASSERT(data->in_buffer != NULL);

  event = data->in_buffer;
  x = ((event[3] >> 4) << 8) | event[4];
  y = ((event[6] & 0xf) << 8) | event[5];

  /* lumio_which_finger(data, &which, x, y); */

  if ((event[3] & (1 << 2)) == (1 << 2))
    which = 0;
  else
    which = 1;

  printk(KERN_INFO "lumio_driver: WHICH: %d\n", which);

  /* This one better works with evdev xorg driver */
  input_report_key(data->fakemouse[which].idev,
		   BTN_LEFT, 0);
  /* input_report_key(data->fakemouse[which].idev, */
  /* 		   BTN_TOUCH, */
  /* 		   !!!((event[3] & LUMIO_OPERATION_MASK) & LUMIO_OPERATION_UP)); */

  /* The code below shouldn't change between xorg drivers. */
  input_report_abs(data->fakemouse[which].idev,
		   ABS_X, x);
  input_report_abs(data->fakemouse[which].idev,
		   ABS_Y, y);
  data->fakemouse[which].last_x = x;
  data->fakemouse[which].last_y = y;

  if (event_type == LUMIO_DUAL_EVENT)
    printk(KERN_INFO "lumio_driver:    Event type: Dual touch event\n");
  else
    printk(KERN_INFO "lumio_driver:    Event type: single touch event\n");

  PRINT_BUFF(data->in_buffer, "first 8bytes");
  printk(KERN_INFO "lumio_driver:    Fingers[%d](x, y) = (%d, %d)\n", which, x, y);
  if (!!!((event[3] & LUMIO_OPERATION_MASK) & LUMIO_OPERATION_UP))
    printk(KERN_INFO "lumio_driver:    Operation: DOWN.\n");
  else
    printk(KERN_INFO "lumio_driver:    Operation: UP.\n");

  input_sync(data->fakemouse[which].idev);

  if (event_type == LUMIO_DUAL_EVENT)
    {
      x = ((event[11] & 0xf) << 8) | event[10];
      y = ((event[11] >> 4) << 8) | event[12];

      if ((event[9] & (1 << 6)) == (1 << 6))
	which = 0;
      else
	which = 1;

      printk(KERN_INFO "lumio_driver: WHICH: %d\n", which);
  
      PRINT_BUFF(data->in_buffer + 8, "2nd 8bytes");
      printk(KERN_INFO "lumio_driver:    Fingers[%d](x, y) = (%d, %d)\n", which, x, y);
      if (!!!((event[9] >> 4) & LUMIO_OPERATION_UP))
	printk(KERN_INFO "lumio_driver:    Operation: DOWN.\n");
      else
	printk(KERN_INFO "lumio_driver:    Operation: UP.\n");

      /* This one better works with evdev xorg driver */
      input_report_key(data->fakemouse[which].idev,
      		       BTN_LEFT, 0);
      /* input_report_key(data->fakemouse[which].idev, */
      /* 		       BTN_LEFT, */
      /* 		       !!!((event[9] >> 4) & LUMIO_OPERATION_UP)); */

      /* The code below shouldn't change between xorg drivers. */
      input_report_abs(data->fakemouse[which].idev,
		       ABS_X, x);
      input_report_abs(data->fakemouse[which].idev,
		       ABS_Y, y);
      data->fakemouse[which].last_x = x;
      data->fakemouse[which].last_y = y;
      input_sync(data->fakemouse[which].idev);
    }
}

/**
 * @brief Reports events from the touchscreen to the kernel input layer.
 *
 *	This function is called in a interrupt context, when the urb has
 * arrived. This function asks the controller for the event report and
 * translate it into events which are passed to the input layer of the kernel.
 *
 *	The report takes the form of two packets, 8 bytes each which will be
 * described later.
 * @param urb The urb that has been raised.
 */
static void			lumio_irq1(struct urb* urb)
{
  struct usb_touchscreen*	data;

  ASSERT(urb != NULL);
  ASSERT(urb->context != NULL);

  if (urb->status != 0)
    return;

  printk(KERN_INFO "lumio_driver: IRQ1 fired!\n");

  data = urb->context;
  if (data->firmware_version != LUMIO_FIRMWARE_3_0)
    usb_submit_urb(data->urb_in2, GFP_ATOMIC);
  else
    {
      if (data->in_buffer[2] == 0x0d)
	lumio_treat_event(data, LUMIO_DUAL_EVENT);
      else
	lumio_treat_event(data, LUMIO_SINGLE_EVENT);
      usb_submit_urb(data->urb_in, GFP_ATOMIC);
    }
}

static void			lumio_irq2(struct urb* urb)
{
  struct usb_touchscreen*	data;

  ASSERT(urb != NULL);
  ASSERT(urb->context != NULL);

  if (urb->status != 0)
    return;

  data = urb->context;

  if (data->in_buffer[2] == 0x0d)
    lumio_treat_event(data, LUMIO_DUAL_EVENT);
  else
    lumio_treat_event(data, LUMIO_SINGLE_EVENT);

  usb_submit_urb(data->urb_in, GFP_ATOMIC);
}

/**
 * @brief Initializes almost everything.
 *
 *	This functions registers two input devices which are the two fake mice,
 * and the urb needed to communicate with the device.
 *
 * @param data Driver's internal datas.
 * @return 0 on success, a negative number if it fails.
 */
static int		lumio_init_data(struct usb_touchscreen* data)
{
  int interval = 0;

  ASSERT(data != NULL);

  if (!(data->urb_in = usb_alloc_urb(0, GFP_KERNEL)))
    goto error;

  if (data->firmware_version == LUMIO_FIRMWARE_3_0)
    {
      interval = 2;
      if (!(data->urb_commander = usb_alloc_urb(0, GFP_KERNEL)))
	goto error;
      
      usb_fill_int_urb(data->urb_commander, data->udev,
		       usb_sndintpipe(data->udev, data->int_out_endpoint),
		       data->out_buffer,
		       64, lumio_send_64_aknowledged,
		       data, interval);
      usb_fill_int_urb(data->urb_in, data->udev,
		       usb_rcvintpipe(data->udev, data->int_in_endpoint),
		       data->in_buffer,
		       64, lumio_irq1,
		       data, interval);
    }
  else
    {
      interval = 10;
      if (!(data->urb_in2 = usb_alloc_urb(0, GFP_KERNEL)))
	goto error;

      usb_fill_int_urb(data->urb_in, data->udev,
		       usb_rcvintpipe(data->udev, data->int_in_endpoint),
		       data->in_buffer,
		       0x08, lumio_irq1,
		       data, interval);
      usb_fill_int_urb(data->urb_in2, data->udev,
		       usb_rcvintpipe(data->udev, data->int_in_endpoint),
		       data->in_buffer + 8,
		       0x08, lumio_irq2,
		       data, interval);

    }

  if (!(data->fakemouse[0].idev = input_allocate_device()))
    goto error;
  if (!(data->fakemouse[1].idev = input_allocate_device()))
    goto error1;
  INIT_FAKEMOUSE(data->fakemouse[0].idev, data, idev_name1);
  INIT_FAKEMOUSE(data->fakemouse[1].idev, data, idev_name2);
  if (input_register_device(data->fakemouse[0].idev))
    goto error2;
  if (input_register_device(data->fakemouse[1].idev))
    goto error3;

  return (0);

 error3:
  input_free_device(data->fakemouse[1].idev);
  data->fakemouse[1].idev = NULL;
  goto error;
 error2:
  input_free_device(data->fakemouse[1].idev);
  data->fakemouse[1].idev = NULL;
 error1:
  input_free_device(data->fakemouse[0].idev);
  data->fakemouse[0].idev = NULL;
 error:
  return (-ENOMEM);
}

static int	lumio_probe_firmware(usb_touchscreen_t*			data,
			     const struct usb_device_id*	entity)
{
 if (entity->idVendor != USB_VID_LUMIO)
    {
      data->firmware_version = LUMIO_FIRMWARE_1_0;
      if (entity->idProduct == USB_PID_DM)
	data->cur_mode = USB_DRIVER_MODE;
    }
  else
    {
      switch (entity->idProduct)
	{
	case USB_PID_DM_2_0:
	  data->cur_mode = USB_DRIVER_MODE;
	case USB_PID_MM_2_0:
	  data->send_msg = lumio_send_8_bytes;
	  data->firmware_version = LUMIO_FIRMWARE_2_0;
	  break;
	case USB_PID_DUAL_MODE_3_0:
	case USB_PID_4_SENSORS_3_0:
	  data->send_msg = lumio_send_64_bytes;
	  data->firmware_version = LUMIO_FIRMWARE_3_0;
	  break;
	default:
	  return (-ENODEV);
	  break;
	}
    }

 return (0);
}

/**
 * @brief Called when a lumio touchscreen is plugged.
 *
 *	This function is called when we plug a lumio touchscrenn. We allocate
 * all private data we need and discover the endpoint which will be used to
 * communicate with the device. The lumio touch screen may be in one of two
 * states :
 *
 * - Mouse mode: In this mode the lumio touchscreen usb controller emulates a
 *   usb mouse. Dual touch is not supported in this mode.
 * - Driver mode: Here's the mode that we want as the controller does not
 *   emulate a usb mouse. This mode supports two configuration, single touch
 *   mode or dual touch mode.
 *
 *	When first plugged, the controller is in mouse mode, so the first thing
 * the driver does is to tell the controller to switch to driver mode (see
 * lumio_switch_to_driver_mode()).
 *	After the controller is in driver mode, it will only notice single
 * touch events, so have to tell it we'd like to receive dual touch events (see
 * lumio_switch_to_dualtouch_mode()).
 *	Now that the controller will actually sends us dual touch events, we
 * have to set up an interrupt urb for it to wake the driver each time it has
 * an event to communicate (see teton()).
 *
 * @param interface
 * @param entity
 * @return 0 on success, a negative number on failure.
 */
static int __devinit		lumio_probe(struct usb_interface*	interface,
					    const struct usb_device_id*	entity)
{
  int				ret = 0;
  struct usb_touchscreen*			data;

  data = kzalloc(sizeof(struct usb_touchscreen), GFP_KERNEL);
  if (!data)
    {
      printk(KERN_WARNING "lumio_driver: unable to allocate private structure.\n");
      ret = -ENOMEM;
      goto error;
    }

  kref_init(&data->refcount);

  usb_set_intfdata(interface, data);

  data->udev = usb_get_dev(interface_to_usbdev(interface));
  data->interface = interface;
  data->cur_mode = USB_MOUSE_MODE;
  data->in_buffer = kzalloc(64, GFP_KERNEL);
  data->out_buffer = kzalloc(64, GFP_KERNEL);
  data->recv_msg = lumio_recv_8_bytes;
  if (!data->in_buffer || !data->out_buffer)
    {
      printk(KERN_WARNING "lumio_driver: unable to allocate interrupt in buffer.\n");
      ret = -ENOMEM;
      goto error;
    }

  SAFE_CALL(lumio_find_endpoints(data), "Cannot find endpoints.\n");

  SAFE_CALL(lumio_probe_firmware(data, entity),
	    "Device not supported.\n");

  switch (data->firmware_version)
    {
    case LUMIO_FIRMWARE_1_0:
    case LUMIO_FIRMWARE_2_0:
      if (data->cur_mode == USB_MOUSE_MODE)
	{
	  printk(KERN_INFO "lumio_driver: Mouse mode.\n");
	  SAFE_CALL(lumio_switch_to_driver_mode(data),
		    "Cannot switch to driver mode.\n");
	  return (0);
	}
    case LUMIO_FIRMWARE_3_0:
      printk(KERN_INFO "lumio_driver: Driver mode.\n");
      SAFE_CALL(usb_register_dev(interface, &lumio_class),
		"Unable to get a minor.\n");

      SAFE_CALL(lumio_init_data(data),
		"Unable to allocate input devices (fakemice).\n");

      SAFE_CALL(lumio_switch_to_dualtouch_mode(data),
		"Unable to switch to dual touch mode.\n");

      printk(KERN_INFO "lumio_driver: Set to dual control.\n");
      break;
    }

  return (0);

 error:

  if (data)
    kref_put(&data->refcount, lumio_delete);

  return (ret);
}

/**
 * @brief Called if the driver is rmmoded or when the device has been unplugged.
 *
 *	This function is called when someone unplug the device. Its work is to
 * remove the /dev/lumio char device and to remove all allocated data. If the
 * interface is still plugged (that mean the driver has been rmmoded) the
 * driver sets back the controller to mouse mode (see
 * lumio_switch_to_mouse_mode()).
 *
 * @param interface
 */
static void __devexit		lumio_disconnect(struct usb_interface* interface)
{
  struct usb_touchscreen*	data;

  lock_kernel();

  data = usb_get_intfdata(interface);
  usb_set_intfdata(interface, NULL);

  if (data && data->cur_mode == USB_DRIVER_MODE)
    usb_deregister_dev(interface, &lumio_class);

  unlock_kernel();

  if (data)
    kref_put(&data->refcount, lumio_delete);

  printk(KERN_INFO "lumio_driver: device unplugged.\n");
}

static struct usb_driver	lumio_driver =
  {
    .name	= "lumio_driver",
    .id_table	= lumio_id_table,
    .probe	= lumio_probe,
    .disconnect	= lumio_disconnect,
  };

/**
 * @brief Called when modprobing the driver.
 *
 *	This function is called when we register this driver (modprobe /
 * insmode).  It tells the kernel that this is a usb driver and which devices
 * this driver will be supporting.
 *
 * @return 0 on success, a negative number on failure.
 */
static int __init		lumio_init(void)
{
  int			ret = 0;

  SAFE_CALL(usb_register(&lumio_driver),
	    "Unable to register lumio touchscreen driver.\n");
  return (0);

 error:

  return (-ret);
}

/**
 * @brief Called when rmmoding the driver.
 *
 *	This function unregister (rmmode) the lumio touchscreen driver from the
 * linked list of usb drivers in the kernel.
 */
static void __exit		lumio_exit(void)
{
  usb_deregister(&lumio_driver);
}

module_init(lumio_init);
module_exit(lumio_exit);
