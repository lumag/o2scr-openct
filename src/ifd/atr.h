/*
 * ATR type definitions
 *
 * Copyright (C) 2004, Olaf Kirch <okir@suse.de>
 */

#ifndef OPENCT_ATR_H
#define OPENCT_ATR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ifd_atr_info {
	/* The following contain -1 if the field wasn't present */
	int		TA[3];
	int		TB[3];
	int		TC[3];
	unsigned int	supported_protocols;
	int		default_protocol;
} ifd_atr_info_t;

extern int	ifd_atr_parse(ifd_atr_info_t *, const unsigned char *, size_t);
extern int	ifd_build_pts(const ifd_atr_info_t *, int,
				unsigned char *, size_t);

#ifdef __cplusplus
}
#endif

#endif /* OPENCT_ATR_H */
