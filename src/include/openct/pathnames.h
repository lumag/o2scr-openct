/*
 * Compile time configuration for libifd
 * FIXME - should move to autoconf
 *
 * Copyright (C) 2003 Olaf Kirch <okir@suse.de>
 */

#ifndef OPENCT_CONFIG_H
#define OPENCT_CONFIG_H

#define OPENCT_CONFIG_PATH	"/etc/openct.conf"
#define OPENCT_MODULES_PATH	"/usr/lib/ifd"
#define OPENCT_SOCKET_PATH	"/var/run/openct"
#define OPENCT_STATUS_PATH	OPENCT_SOCKET_PATH "/status"
#define OPENCT_IFDHANDLER_PATH	"/usr/sbin/openct-ifdhandler"

#endif /* OPENCT_CONFIG_H */
