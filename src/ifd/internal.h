/*
 * Core functions of the IFD handler library
 *
 */

#ifndef IFD_INTERNAL_H
#define IFD_INTERNAL_H

#include "core.h"

struct ifd_device {
	const char *		name;
	int			flags;
	struct ifd_device_ops *	ops;
	/* data follows */
};

#define IFD_DEVICE_BLKIO	0x0001	/* device does block I/O (e.g. USB) */

struct ifd_device_ops {
	/* Try to identify the attached device. In the case of
	 * a serial device, perform serial PnP. For USB devices,
	 * get the vendor/device ID */
	int			(*identify)(ifd_device_t *, char *, size_t);

	/* Reset device */
	int			(*reset)(ifd_device_t *, void *, size_t);

	int			(*set_params)(ifd_device_t *, const ifd_device_params_t *);
	int			(*get_params)(ifd_device_t *, ifd_device_params_t *);

	/*
	 * Send/receive data. Some devices such as USB will support
	 * the transceive command, others such as serial devices will
	 * need to use send/recv
	 */
	int			(*transceive)(ifd_device_t *, ifd_apdu_t *, long);
	int			(*send)(ifd_device_t *, const void *, size_t);
	int			(*recv)(ifd_device_t *, void *, size_t, long);

	int			(*close)(ifd_device_t *);
};

struct ifd_protocol {
	int			id;
	const char *		name;
	int			(*attach)(ifd_reader_t *);
	void			(*detach)(ifd_reader_t *);
	int			(*set_param)(ifd_reader_t *, int, long);
	int			(*get_param)(ifd_reader_t *, int, long *);
	int			(*transceive)(ifd_reader_t *, unsigned char,
						ifd_apdu_t *);
};

extern ifd_protocol_t		t1_protocol;

struct ifd_driver_ops {
	int			(*set_protocol)(ifd_reader_t *, int);
	int			(*transceive)(ifd_reader_t *, ifd_apdu_t *);
};

typedef struct ifd_buf {
	unsigned char *		base;
	unsigned int		head, tail, size;
} ifd_buf_t;

extern ifd_driver_t *	ifd_get_driver(const char *);

extern ifd_device_t *	ifd_open_serial(const char *);
extern ifd_device_t *	ifd_open_psaux(const char *);
extern ifd_device_t *	ifd_open_usb(const char *);

extern int		ifd_device_identify(ifd_device_t *, char *, size_t);
extern int		ifd_device_transceive(ifd_device_t *, ifd_apdu_t *, long);
extern int		ifd_device_send(ifd_device_t *, const void *, size_t);
extern int		ifd_device_recv(ifd_device_t *, void *, size_t, long);
extern void		ifd_device_close(ifd_device_t *);

/* Checksum functions */
extern unsigned int	csum_lrc_compute(const unsigned char *, size_t, unsigned char *);
extern unsigned int	csum_crc_compute(const unsigned char *, size_t, unsigned char *);

/* Buffer handling */
extern void		ifd_buf_init(ifd_buf_t *, void *, size_t);
extern void		ifd_buf_clear(ifd_buf_t *);
extern int		ifd_buf_get(ifd_buf_t *, void *, size_t);
extern int		ifd_buf_put(ifd_buf_t *, const void *, size_t);
extern unsigned int	ifd_buf_avail(ifd_buf_t *);

/* Error handling */
extern void		ifd_error(const char *, ...);

#endif /* IFD_INTERNAL_H */
