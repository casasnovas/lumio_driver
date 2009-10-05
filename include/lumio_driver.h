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
 * @file lumio_driver.h
 * @author Quentin Casasnovas
 * @brief Public header file.
 *
 *	This file is needed for programmers who will implement a userland
 * programm interracting with the driver. You'll find all IOCTL commands
 * defined here.
 */

#ifndef LUMIO_DRIVER_H_
# define LUMIO_DRIVER_H_

# define IOCTL_SET_DELAY_CLIC	0x01
# define IOCTL_SET_SINGLETOUCH	0x02
# define IOCTL_SET_DUALTOUCH	0x03
# define IOCTL_SET_SOUND_ON	0x04
# define IOCTL_SET_SOUND_OFF	0x05

#endif /* !LUMIO_DRIVER_H_ */
