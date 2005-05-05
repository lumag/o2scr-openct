/*
 * Shared status file for OpenCT readers
 *
 * Copyright (C) 2003 Olaf Kirch <okir@suse.de>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <openct/openct.h>
#include <openct/logging.h>

static int		ct_status_lock(void);
static void		ct_status_unlock(void);

static void *
ct_map_status(int flags, size_t *size)
{
	struct stat	stb;
	int		fd, prot;
	void		*addr = NULL;
	char		status_path[1024];

#if defined (sunray) || defined (sunrayclient)
	char		*utdevroot=getenv("UTDEVROOT");

	if(utdevroot)
		snprintf(status_path, sizeof(status_path),
			"%s/openct/status", utdevroot);
	else
		snprintf(status_path, sizeof(status_path), OPENCT_STATUS_PATH);
#else
	snprintf(status_path, sizeof(status_path), OPENCT_STATUS_PATH);
#endif

	if ((fd = open(status_path, flags)) < 0) {
		/* no error message - openct not started? */
		return NULL;
	}

	if (fstat(fd, &stb) < 0) {
		ct_error("unable to stat %s: %m", status_path);
		goto done;
	}
	*size = stb.st_size;

	prot = PROT_READ;
	if ((flags & O_ACCMODE) == O_RDWR)
		prot |= PROT_WRITE;
	
	addr = mmap(0, *size, prot, MAP_SHARED, fd, 0);

done:	close(fd);
	return addr;
}

int
ct_status_clear(unsigned int count)
{
	int	fd;
	char	status_path[1024];

#if defined (sunray) || defined (sunrayclient)
	char	*utdevroot=getenv("UTDEVROOT");

	if(utdevroot)
		snprintf(status_path, sizeof(status_path),
			"%s/openct/status", utdevroot);
	else
		snprintf(status_path, sizeof(status_path), OPENCT_STATUS_PATH);
#else
	snprintf(status_path, sizeof(status_path), OPENCT_STATUS_PATH);
#endif
	unlink(status_path);
	if ((fd = open(status_path, O_RDWR|O_CREAT, 0644)) < 0
	 || ftruncate(fd, count * sizeof(ct_info_t)) < 0
	 || fchmod(fd, 0644) < 0) {
		ct_error("cannot create %s: %m", status_path);
		unlink(status_path);
		if (fd >= 0)
			close(fd);
		return -1;
	}

	return 0;
}

int
ct_status(const ct_info_t **result)
{
	static const ct_info_t *reader_status;
	static unsigned int	num_status;

	if (reader_status == NULL) {
		size_t	size;

		reader_status = (ct_info_t *) ct_map_status(O_RDONLY, &size);
		if (reader_status == NULL)
			return -1;
		num_status = size / sizeof(ct_info_t);
	}

	*result = reader_status;
	return num_status;
}

ct_info_t *
ct_status_alloc_slot(unsigned int *num)
{
	ct_info_t	*info;
	size_t		size;
	unsigned int	n, max;

	info = (ct_info_t *) ct_map_status(O_RDWR, &size);
	if (info == NULL)
		return NULL;

	max = size / sizeof(ct_info_t);
	if (*num == (unsigned int) -1) {
		sigset_t	sigset;

		/* Block all signals while holding the lock */
		sigfillset(&sigset);
		sigprocmask(SIG_SETMASK, &sigset, &sigset);

		/* Lock the status file against concurrent access */
		ct_status_lock();

		/* find a free slot */
		for (n = 0; n < max; n++, info) {
			if (info[n].ct_pid == 0
			 || (kill(info[n].ct_pid, 0) < 0 && errno == ESRCH)) {
				*num = n;
				break;
			}
		}

		/* Done, unlock the file again */
		ct_status_unlock();

		/* unblock signals */
		sigprocmask(SIG_SETMASK, &sigset, NULL);
	} else if (*num >= max) {
		munmap((void *) info, size);
		return NULL;
	}

	memset(&info[*num], 0, sizeof(ct_info_t));
	info[*num].ct_pid = getpid();
	
	msync((void *) info, size, MS_SYNC);
	return info + *num;
}


#define ALIGN(x, size)	(((caddr_t) (x)) - ((unsigned long) (x) % (size)))
int
ct_status_update(ct_info_t *status)
{
	size_t	size;
	caddr_t	page;

	/* get the page this piece of data is sitting on */
	size = getpagesize();
	page = ALIGN(status, size);

	/* flush two pages if data spans two pages */
	if (page != ALIGN(status + 1, size))
		size <<= 1;

	if (msync(page, size, MS_SYNC) < 0) {
		ct_error("msync: %m");
		return -1;
	}

	return 0;
}

/*
 * Lock file handling
 */
int
ct_status_lock(void)
{
	int	fd, retries = 10;
	char	status_lock_path[1024];
	char	locktemp[1024];

#if defined (sunray) || defined (sunrayclient)
	char	*utdevroot=getenv("UTDEVROOT");

	if(utdevroot)
		snprintf(status_lock_path, sizeof(status_lock_path),
			"%s/openct/status.lock", utdevroot);
	else
		snprintf(status_lock_path, sizeof(status_lock_path),
			OPENCT_STATUS_PATH ".lock");
#else
	snprintf(status_lock_path, sizeof(status_lock_path),
		OPENCT_STATUS_PATH ".lock");
#endif
	snprintf(locktemp, sizeof(locktemp),
		"%s.%u", status_lock_path, (unsigned int) getpid());

	if ((fd = open(locktemp, O_CREAT|O_RDWR, 0600)) < 0)
		return -1;

	while (retries--) {
		if (link(locktemp, status_lock_path) >= 0) {
			unlink(locktemp);
			return 0;
		}
	}

	unlink(locktemp);
	return -1;
}

void
ct_status_unlock(void)
{
	char	status_lock_path[1024];

#if defined (sunray) || defined (sunrayclient)
	char	*utdevroot=getenv("UTDEVROOT");

	if(utdevroot)
		snprintf(status_lock_path, sizeof(status_lock_path),
			"%s/openct/status.lock", utdevroot);
	else
		snprintf(status_lock_path, sizeof(status_lock_path),
			OPENCT_STATUS_PATH ".lock");
#else
	snprintf(status_lock_path, sizeof(status_lock_path),
		OPENCT_STATUS_PATH ".lock");
#endif
	unlink(status_lock_path);
}
