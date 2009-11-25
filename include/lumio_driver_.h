/*
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
 * @file lumio_driver_.h
 * @author Quentin Casasnovas
 * @brief Private header file.
 *
 *	This file contains all types, macro and constant that are used
 * internally by the driver. It also includes everything needed to devellop a
 * usb driver for the linux kernel.
 */

#ifndef LUMIO_DRIVER__H_
# define LUMIO_DRIVER__H_

# include <asm/uaccess.h>

# include <linux/usb/input.h>
# include <linux/smp_lock.h>
# include <linux/kthread.h>
# include <linux/kernel.h>
# include <linux/module.h>
# include <linux/input.h>
# include <linux/errno.h>
# include <linux/sched.h>
# include <linux/slab.h>
# include <linux/init.h>
# include <linux/usb.h>
# include <linux/fs.h>

# include "lumio_driver.h"

/*
 * defines
 */


/** @brief The USB Vendor ID of the device when in mouse mode. */
# define USB_VID_MM		0x0556
/** @brief The USB Vendor ID of the device when in driver mode. */
# define USB_VID_DM		0x0592
/** @brief The USB Product ID of the device when in mouse mode. */
# define USB_PID_MM		0x3556
/** @brief The USB Product ID of the device when in driver mode. */
# define USB_PID_DM		0x6956
/** @brief The USB Vendor ID of Lumio (since firmware 2.0) */
# define USB_VID_LUMIO		0x202E
/** @brief The USB PID of the device when in driver mode (firmware 2.0). */
# define USB_PID_DM_2_0			0x0001
/** @brief The USB PID of the device when in mouse mode (firmware 2.0). */
# define USB_PID_MM_2_0			0x0002
/** @brief The USB PID of the device when in dualcontrol mode (firmware 2.0). */
# define USB_PID_DUAL_CONTROL_2_0	0x0003
/** @brief The USB PID of the device when in digitizer mode (firmware 2.0). */
# define USB_PID_DIGITIZER_2_0		0x0004
/** @brief The USB PID of the device when in dual mode mode (firmware 3.0). */
# define USB_PID_DUAL_MODE_3_0		0x0005
/** @brief The USB PID of the device when in 4 sensors mode mode (firmware 3.0). */
# define USB_PID_4_SENSORS_3_0		0x0006


/** @brief The touch screen is in mouse mode. */
# define USB_MOUSE_MODE		(1 << 0)
/** @brief The touch screen is in driver mode. */
# define USB_DRIVER_MODE	(1 << 1)

/** @brief The touch screen is in driver mode, with dualtouch reports. */
# define USB_DUALTOUCH_CONFIG	(1 << 0)
/** @brief The touch screen is in mouse mode, with singletouch reports. */
# define USB_SINGLETOUCH_CONFIG	(1 << 1)


# define HID_REQ_GET_REPORT	0x01
# define HID_REQ_SET_REPORT	0x09
# define HID_VAL_INPUT		0x0100
# define HID_VAL_OUTPUT		0x0200
# define HID_VAL_FEATURE	0x0300

# define LUMIO_OPERATION_MASK	0x3
# define LUMIO_OPERATION_MOVE	(0 << 0)
# define LUMIO_OPERATION_DOWN	(1 << 0)
# define LUMIO_OPERATION_UP	(1 << 1)

# define LUMIO_TAGID_MASK	0xc
# define LUMIO_TAGID_EVENT_ID1	(1 << 2)
# define LUMIO_TAGID_EVENT_ID2	(1 << 3)

# define LUMIO_FIRMWARE_1_0	0x01
# define LUMIO_FIRMWARE_2_0	0x02
# define LUMIO_FIRMWARE_3_0	0x03


# define LUMIO_SINGLE_EVENT	0
# define LUMIO_DUAL_EVENT	1

/*
 * macros
 */

/**
 * @brief Checks if the endpoint is an interrupt one.
 *
 *	Checks whether the endpoint which owns Attr attributes is an interrupt
 * endpoint.
 *
 * @param Attr bmAttributes as defines in the usb specification 2.0 attached to
 * the endpoint.
 */
# define IS_INT_ENDPOINT(Attr)						\
  ((Attr & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)

/**
 * @brief Calls Func in a proper way.
 *
 *	This is a helper macro function used to save recurrent test of the
 * return value of a function. If the return value of Func is 0 then nothing
 * happen, but if there was an error, Error string is passed to the kernel and
 * the caller should terrminate properly in the label "error:".
 *
 * @param Func The function to call properly.
 * @param Error The string that will be printed in dmesg in the case of an
 * error has raised when calling Func
 */
# define SAFE_CALL(Func, Error)						\
  do									\
    {									\
      if ((ret = (Func)) < 0)						\
	{								\
	  printk(KERN_WARNING "lumio_driver: " Error);			\
	  goto error;							\
	}								\
    } while (0);

/**
 * @brief Aborts the program if assertion is false.
 *
 *	This macro function assures that a certain assertion, Expr, is true. If
 * this is not the case, it's a bug in the driver and the execution is stopped
 * sending the kernel usefull informations on where the problem happen.
 *
 * @param Expr The expression to check.
 */
# define ASSERT(Expr)							\
  do									\
    {									\
      if (!(Expr))							\
	{								\
	  printk(KERN_WARNING						\
		 "lumio_driver: Assertion failed! %s, %s, %s,line=%d\n",	\
		 #Expr, __FILE__, __func__, __LINE__);			\
	  BUG();							\
	}								\
    } while (0);

/**
 * @brief Init a fake input device.
 *
 *	This macro function takes care of initializing all needed values from
 * the input_dev structure as to be able to register our fake mice. Note that
 * we register ABS_X/Y and not relatives one as a normal mouse would.
 * @param Inputdev The input_dev struct to initialize.
 * @param Data The touchscreen.
 */
# define INIT_FAKEMOUSE(Inputdev, Data, Name)				\
  do									\
    {									\
      (Inputdev)->name = (Name);					\
      input_set_drvdata((Inputdev), (Data));				\
      usb_to_input_id((Data)->udev, &(Inputdev)->id);			\
      set_bit(EV_KEY, (Inputdev)->evbit);				\
      set_bit(BTN_LEFT, (Inputdev)->keybit);				\
      set_bit(EV_ABS, (Inputdev)->evbit);				\
      if (data->firmware_version == LUMIO_FIRMWARE_1_0)			\
	{								\
	  input_set_abs_params((Inputdev), ABS_X, 0, 2047, 0, 0);	\
	  input_set_abs_params((Inputdev), ABS_Y, 0, 2047, 0, 0);	\
	}								\
      else if (data->firmware_version == LUMIO_FIRMWARE_2_0 ||		\
	       data->firmware_version == LUMIO_FIRMWARE_3_0)		\
	{								\
	  input_set_abs_params((Inputdev), ABS_X, 0, 4095, 0, 0);	\
	  input_set_abs_params((Inputdev), ABS_Y, 0, 4095, 0, 0);	\
	}								\
      (Inputdev)->open = lumio_fake_open;				\
      (Inputdev)->close = lumio_fake_close;				\
    } while (0);							
/**
 * @brief Print an 8bytes buffer and its description
 *
 *	Helper function to print an 8bytes buffer with a small description
 * before it. The buffer is printed in hex. It's only used internally for debug
 * purposes.
 * @param Buff The buffer to print.
 * @param Desc A small description.
 */
# define PRINT_BUFF(Buff, Desc)						\
  do									\
    {									\
      printk(KERN_INFO "lumio_driver: " Desc				\
	     " 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",			\
	     (Buff)[0], (Buff)[1], (Buff)[2], (Buff)[3],		\
	     (Buff)[4], (Buff)[5], (Buff)[6], (Buff)[7]);		\
    } while (0);							\


# define PRINT_RECEIVED_DEBUG_TRACE()					\
  printk(KERN_INFO "lumio_driver:\tEvent type: ");			\
  if (event_type == LUMIO_DUAL_EVENT)					\
    printk(KERN_INFO "dualtouch event\n");				\
  else									\
    printk(KERN_INFO "singletouch event\n");				\
  PRINT_BUFF(data->in_buffer, "first 8bytes");				\
  printk(KERN_INFO							\
	 "lumio_driver:\tFingers[%d](x, y) = (%d, %d)\n",		\
	 which, x, y);							\
  if (up)								\
    printk(KERN_INFO "lumio_driver:    Operation: DOWN.\n");		\
  else									\
    printk(KERN_INFO "lumio_driver:    Operation: UP.\n");

/*
 * types
 */

/**
 * @brief Represents a fakemouse
 *
 *	This structure is used internally by the driver to track the
 * fingers. It permits to guess which fakemouse is closer to the touch, it
 * simulates that we kept the position of the fingers even when it weren't on
 * the touchscreen.
 */
typedef struct			usb_fakemouse
{
  struct input_dev*		idev;
  __u16				last_x;
  __u16				last_y;
}				usb_fakemouse_t;

/**
 * @brief Internally datas used by the driver.
 *
 *	Represents the driver internally data that are used to extracts events
 * from the lumio touchscreen, and send it control commands.
 */
typedef struct			usb_touchscreen
{
  struct usb_interface*		interface; /**< Interface registered to the driver. */
  struct usb_fakemouse		fakemouse[2]; /**< The two fake mice devices. */
  struct usb_device*		udev; /**< Usb device registered to the driver. */
  struct urb*			urb_in; /**< A urb to communicate with the controller. */
  struct urb*			urb_in2;
  struct urb*			urb_commander;
  struct kref			refcount; /**< Reference counter. */
  unsigned char*		out_buffer; /**< Buffer used to send data to ts. */
  unsigned char*		in_buffer; /**< Buffer used to retreive data from ts. */
  __u8				int_out_endpoint; /**< Interrupt endpoint of the device (Out). */
  __u8				int_in_endpoint; /**< Interrupt endpoint of the device (In). */
  __u8				listeners; /**< Numbers of listeners of our fake mice events. */
  __u8				cur_mode; /**< The current mode of the device. */
  __u8				firmware_version; /**< The firmware version of the controller. */

  int				(*send_msg)(struct usb_touchscreen*, unsigned int);
  int				(*recv_msg)(struct usb_touchscreen*, unsigned int);
}				usb_touchscreen_t;

#endif /* !LUMIO_DRIVER__H_ */
