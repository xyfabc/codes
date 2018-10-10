
How to Reset USB Device in Linux
by ROMAN10 on MAY 4, 2011 · 7 COMMENTS
Tweet
Share

USB devices are anywhere nowadays, even many embedded devices replace the traditional serial devices with usb devices. However, I experienced that USB devices hang from time to time. In most cases, a manual unplug and replug will solve the issue. Actually, usb reset can simulate the unplug and replug operation.

First, get the device path for your usb device. Enter the command lsusb will give you something similar as below,

Bus 008 Device 001: ID 1d6b:0001 Linux Foundation 1.1 root hub
Bus 007 Device 001: ID 1d6b:0001 Linux Foundation 1.1 root hub
Bus 006 Device 002: ID 04b3:310c IBM Corp. Wheel Mouse
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 004 Device 002: ID 0a5c:2145 Broadcom Corp.
Bus 005 Device 001: ID 1d6b:0001 Linux Foundation 1.1 root hub

Use the IBM Wheel Mouse as an example, the device node for it is /dev/bus/usb/006/002, where 006 is the bus number, and 002 is the device number.

Second, apply ioctl operation to reset the device. This is done in C code,

#include <stdio.h>

#include <fcntl.h>

#include <errno.h>

#include <sys/ioctl.h>

#include <linux/usbdevice_fs.h>

void main(int argc, char **argv)

{

	const char *filename;

	int fd;

	filename = argv[1];

	fd = open(filename, O_WRONLY);

	ioctl(fd, USBDEVFS_RESET, 0);

	close(fd);

	return;

}

Save the code above as reset.c, then compile the code using

gcc -o reset reset.c

This will produce the a binary named reset. Again, using the wheel mouse as an example, execute the following commands,

sudo ./reset /dev/bus/usb/006/002

You can take a look at the message by,

tail -f /var/log/messages

On my Ubuntu desktop, the last line reads,

May 4 16:09:17 roman10 kernel: [ 1663.013118] usb 6-2:
reset low speed USB device using uhci_hcd and address 2

This reset operation is effectively the same as you unplug and replug a usb device.

For another method of reset usb using libusb, please refer here

Reference: http://forums.x-plane.org/index.php?app=downloads&showfile=9485.

