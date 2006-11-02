/*
 * Utility functions
 *
 * Copyright (C) 2003, Olaf Kirch <okir@suse.de>
 */

#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#ifndef __GNUC__
void ifd_debug(int level, const char *fmt, ...)
{
	va_list ap;
	char str[2048];

	va_start(ap, fmt);
	vsnprintf(str, sizeof(str), fmt, ap);
	if (level <= ct_config.debug)
		ct_debug(str);
	va_end(ap);
}
#endif

unsigned int ifd_count_bits(unsigned int word)
{
	static unsigned int bcount[16] = {
		0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
	};
	unsigned int num;

	for (num = 0; word; word >>= 4)
		num += bcount[word & 0xF];
	return num;
}

void ifd_revert_bits(unsigned char *data, size_t len)
{
	unsigned char j, k, c, d;

	while (len--) {
		c = *data;
		for (j = 1, k = 0x80, d = 0; k != 0; j <<= 1, k >>= 1) {
			if (c & j)
				d |= k;
		}
		*data++ = d ^ 0xFF;
	}
}

#ifndef timersub
# define timersub(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;			      \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;			      \
    if ((result)->tv_usec < 0) {					      \
      --(result)->tv_sec;						      \
      (result)->tv_usec += 1000000;					      \
    }									      \
  } while (0)
#endif

/* return time elapsed since "then" in miliseconds */
long ifd_time_elapsed(struct timeval *then)
{
	struct timeval now, delta;

	gettimeofday(&now, NULL);
	timersub(&now, then, &delta);
	return delta.tv_sec * 1000 + (delta.tv_usec / 1000);
}

/*
 * Spawn an ifdhandler
 */
int ifd_spawn_handler(const char *driver, const char *devtype, int idx)
{
	const char *argv[16];
	char reader[16], debug[10];
	char *type, *device;
	int argc, n;
	pid_t pid;

	ifd_debug(1, "driver=%s, devtype=%s, index=%d", driver, devtype, idx);

	if ((pid = fork()) < 0) {
		ct_error("fork failed: %m");
		return 0;
	}

	if (pid != 0) {
		/* We're the parent process. The child process should
		 * call daemon(), causing the process to exit
		 * immediately after allocating a slot in the status
		 * file. We wait for it here to make sure USB devices
		 * don't claim a slot reserved for another device */
		waitpid(pid, NULL, 0);
		return 1;
	}

	argc = 0;
	argv[argc++] = ct_config.ifdhandler;

	if (idx >= 0) {
		snprintf(reader, sizeof(reader), "-r%u", idx);
		argv[argc++] = reader;
	} else {
		argv[argc++] = "-H";
	}

	if (ct_config.debug) {
		if ((n = ct_config.debug) > 6)
			n = 6;
		debug[n + 1] = '\0';
		while (n--)
			debug[n + 1] = 'd';
		debug[0] = '-';
		argv[argc++] = debug;
	}

	type = strdup(devtype);
	device = strtok(type,":");
	device = strtok(NULL,":");
	if (!device || !type) {
		ct_error("failed to parse devtype %s",devtype);
		exit(1);
	}

	argv[argc++] = driver;
	argv[argc++] = type;
	argv[argc++] = device;
	argv[argc] = NULL;

	n = getdtablesize();
	while (--n > 2)
		close(n);

	execv(ct_config.ifdhandler, (char **)argv);
	ct_error("failed to execute %s: %m", ct_config.ifdhandler);
	exit(1);
}

/*
 * Replacement for the BSD daemon() function
 */
#ifndef HAVE_DAEMON
int daemon(int nochdir, int noclose)
{
	pid_t pid;

	pid = fork();

	/* In case of fork is error. */
	if (pid < 0)
		return -1;

	/* In case of this is parent process. */
	if (pid != 0)
		exit(0);

	/* Become session leader and get pid. */
	pid = setsid();

	if (pid < -1) {
		perror("setsid");
		return -1;
	}

	/* Change directory to root. */
	if (!nochdir)
		chdir("/");

	/* File descriptor close. */
	if (!noclose) {
		int fd;

		fd = open("/dev/null", O_RDWR, 0);
		if (fd != -1) {
			dup2(fd, STDIN_FILENO);
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
			if (fd > 2)
				close(fd);
		}
	}
	umask(0027);
	return 0;
}
#endif
