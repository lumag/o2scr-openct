/*
 * I/O routines for pcmcia devices
 *
 * Copyright (C) 2003 Olaf Kirch <okir@lst.de>
 * Copyright (C) 2005 Harald Welte <laforge@gnumonks.org>
 */

#include "internal.h"
#include <sys/types.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

/*
 * Input/output routines
 */
static int ifd_pcmcia_send(ifd_device_t * dev, const unsigned char *buffer,
			   size_t len)
{
	size_t total = len;
	int n;

	while (len) {
		n = write(dev->fd, buffer, len);
		if (n < 0) {
			ct_error("Error writing to %s: %m", dev->name);
			return -1;
		}
		buffer += n;
		len -= n;
	}

	return total;
}

static int ifd_pcmcia_recv(ifd_device_t * dev, unsigned char *buffer,
			   size_t len, long timeout)
{
	size_t total = len, to_read;
	struct timeval begin;
	int n;

	gettimeofday(&begin, NULL);

	while (len) {
		struct pollfd pfd;
		long wait;

		if ((wait = timeout - ifd_time_elapsed(&begin)) < 0)
			goto timeout;

		pfd.fd = dev->fd;
		pfd.events = POLLIN;
		n = poll(&pfd, 1, wait);
		if (n < 0) {
			ct_error("%s: error while waiting for input: %m",
				 dev->name);
			return -1;
		}
		if (n == 0)
			continue;

		to_read = len;

		n = read(dev->fd, buffer, to_read);
		if (n < 0) {
			ct_error("%s: failed to read from device: %m",
				 dev->name);
			return -1;
		}
		if (ct_config.debug >= 9)
			ifd_debug(9, "pcmcia recv:%s", ct_hexdump(buffer, n));
		buffer += n;
		len -= n;
	}

	return total;

      timeout:			/* Timeouts are a little special; they may happen e.g.
				 * when trying to obtain the ATR */
	if (!ct_config.suppress_errors)
		ct_error("%s: timed out while waiting for input", dev->name);
	ifd_debug(9, "(%u bytes received so far)", total - len);
	return IFD_ERROR_TIMEOUT;
}

/*
 * Close the device
 */
static void ifd_pcmcia_close(ifd_device_t * dev)
{
	if (dev->fd >= 0)
		close(dev->fd);
	dev->fd = -1;
}

static struct ifd_device_ops ifd_pcmcia_ops;

/*
 * Open serial device
 */
ifd_device_t *ifd_open_pcmcia(const char *name)
{
	ifd_device_t *dev;
	int fd;

	if ((fd = open(name, O_RDWR)) < 0) {
		ct_error("Unable to open %s: %m", name);
		return NULL;
	}

	ifd_pcmcia_ops.send = ifd_pcmcia_send;
	ifd_pcmcia_ops.recv = ifd_pcmcia_recv;
	ifd_pcmcia_ops.close = ifd_pcmcia_close;

	dev = ifd_device_new(name, &ifd_pcmcia_ops, sizeof(*dev));
	dev->timeout = 1000;	/* acceptable? */
	dev->type = IFD_DEVICE_TYPE_PCMCIA;
	dev->fd = fd;

	return dev;
}
