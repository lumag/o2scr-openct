/*
 * Module handling
 *
 * Copyright (C) 2003 Olaf Kirch <okir@suse.de>
 */

#include "internal.h"
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/param.h>

static const char *
ifd_module_path(const char *subdir)
{
	static char	path[PATH_MAX];

	if (!ct_config.modules_dir
	 && !(ct_config.modules_dir = getenv("IFD_MODULES")))
		ct_config.modules_dir = OPENCT_MODULES_PATH;

	snprintf(path, sizeof(path), "%s/%ss",
			ct_config.modules_dir, subdir);
	return path;
}

int
ifd_load_module(const char *type, const char *name)
{
	const char	*dirname;
	char		path[PATH_MAX];
	void		*handle;
	void		(*init_func)(void);

	if (strstr(name, "..")) {
		ct_error("Illegal module path \"%s\"", name);
		return -1;
	}

	if (!strcmp(type, "driver")) {
		dirname = ct_config.driver_modules_dir;
	} else
	if (!strcmp(type, "protocol")) {
		dirname = ct_config.protocol_modules_dir;
	} else {
		ct_error("Unknown module type \"%s\"", type);
		return -1;
	}

	if (!dirname)
		dirname = ifd_module_path(type);

	snprintf(path, sizeof(path), "%s/%s.so", dirname, name);

#ifdef RTLD_NOW
	handle = dlopen(path, RTLD_NOW);
#else
	handle = dlopen(path, 0);
#endif
	if (!handle) {
		ct_error("Failed to load %s: %s", path, dlerror());
		return -1;
	}

	init_func = (void (*)(void)) dlsym(handle, "ifd_init_module");
	if (!init_func) {
		ct_error("%s: no function called ifd_init_module", path);
		dlclose(handle);
		return -1;
	}

	init_func();
	return 0;
}
