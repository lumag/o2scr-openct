/*
 * Handle TLV encoded data
 *
 * Copyright (C) 2003 Olaf Kirch <okir@suse.de>
 */

#ifndef IFD_TLV_H
#define IFD_TLV_H

#include <openct/protocol.h>
#include <openct/buffer.h>

typedef unsigned char	ifd_tag_t;

typedef struct ct_tlv_parser {
	unsigned char *	v[256];
} ct_tlv_parser_t;

typedef struct ct_tlv_builder {
	int		error;
	ct_buf_t *	buf;
	unsigned char *	lenp;
} ct_tlv_builder_t;

extern int	ct_tlv_parse(ct_tlv_parser_t *, ct_buf_t *);
extern int	ct_tlv_get_int(ct_tlv_parser_t *,
				ifd_tag_t, unsigned int *);
extern int	ct_tlv_get_string(ct_tlv_parser_t *,
				ifd_tag_t, char *, size_t);
extern int	ct_tlv_get_opaque(ct_tlv_parser_t *,
				ifd_tag_t, unsigned char **, size_t *);
extern int	ct_tlv_get_bytes(ct_tlv_parser_t *,
				ifd_tag_t, void *, size_t);

extern void	ct_tlv_builder_init(ct_tlv_builder_t *, ct_buf_t *);
extern void	ct_tlv_put_int(ct_tlv_builder_t *,
				ifd_tag_t, unsigned int);
extern void	ct_tlv_put_string(ct_tlv_builder_t *,
				ifd_tag_t, const char *);
extern void	ct_tlv_put_tag(ct_tlv_builder_t *, ifd_tag_t);
extern void	ct_tlv_add_byte(ct_tlv_builder_t *, unsigned char);
extern void	ct_tlv_add_bytes(ct_tlv_builder_t *,
				const unsigned char *, size_t);

#endif /* IFD_TLV_H */
