/*
 * OpenCT ifdhandler control
 *
 * Copyright (C) 2003 Olaf Kirch <okir@suse.de>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <openct/openct.h>
#include <openct/ifd.h>
#include <openct/conf.h>
#include <openct/driver.h>
#include <openct/logging.h>

static int		mgr_init(int argc, char **argv);
static int		mgr_shutdown(int argc, char **argv);
static int		mgr_attach(int argc, char **argv);
static int		mgr_status(int argc, char **argv);
static void		usage(int exval);

static const char *	opt_config = NULL;
static int		opt_debug = 0;
static int		opt_coldplug = 1;

static void		configure_reader(ifd_conf_node_t *);

int
main(int argc, char **argv)
{
	int	c;

	/* Make sure the mask is good */
	umask(033);

	while ((c = getopt(argc, argv, "df:hs")) != -1) {
		switch (c) {
		case 'd':
			opt_debug++;
			break;
		case 'n':
			opt_coldplug=0;
			break;
		case 'f':
			opt_config = optarg;
			break;
		case 'h':
			usage(0);
		default:
			usage(1);
		}
	}

	ct_config.debug = opt_debug;

	/* Parse IFD config file */
	if (ifd_config_parse(opt_config) < 0)
		exit(1);

	if (optind == argc)
		usage(1);

	argv += optind;
	argc -= optind;

	if (!strcmp(argv[0], "init")) {
		return mgr_init(argc, argv);
	} else
	if (!strcmp(argv[0], "shutdown")) {
		return mgr_shutdown(argc, argv);
	} else
	if (!strcmp(argv[0], "attach")) {
		return mgr_attach(argc, argv);
	} else
	if (!strcmp(argv[0], "status")) {
		return mgr_status(argc, argv);
	}

	fprintf(stderr, "Unknown command: %s\n", argv[0]);
	return 1;
}

int
mgr_init(int argc, char **argv)
{
	int	n;

	if (argc != 1)
		usage(1);

	/* Zap the status file */
	ct_status_clear(OPENCT_MAX_READERS);

	/* Initialize IFD library */
	ifd_init();

	/* Create an ifdhandler process for every reader defined
	 * in the config file */
	n = ifd_conf_get_nodes("reader", NULL, 0);
	if (n >= 0) {
		ifd_conf_node_t	**nodes;
		int		i;

		nodes = (ifd_conf_node_t **) calloc(n, sizeof(*nodes));
		n = ifd_conf_get_nodes("reader", nodes, n);
		for (i = 0; i < n; i++)
			configure_reader(nodes[i]);
		free(nodes);
	}

	/* Create an ifdhandler process for every hotplug reader found */
	if (opt_coldplug)
		ifd_scan_usb();
	return 0;
}

/*
 * shut down the whole thing
 */
int
mgr_shutdown(int argc, char **argv)
{
	const ct_info_t	*status;
	int		num, killed = 0;

	if (argc != 1)
		usage(1);

	if ((num = ct_status(&status)) < 0) {
		fprintf(stderr,
			"cannot access status file; no readers killed\n");
		return 1;
	}

	while (num--) {
		if (status[num].ct_pid
		 && kill(status[num].ct_pid, SIGTERM) >= 0)
			killed++;
	}

	printf("%d process%s killed.\n", killed, (killed == 1)? "" : "es");
	return 0;
}

/*
 * Attach a new reader
 */
int
mgr_attach(int argc, char **argv)
{
	const char	*device, *driver, *idstring;
	ifd_devid_t	id;
	pid_t		pid;

	if (argc != 3)
		usage(1);
	device = argv[1];
	idstring = argv[2];

	/* Initialize IFD library */
	ifd_init();

	if (ifd_device_id_parse(idstring, &id) < 0) {
		fprintf(stderr, "Cannot parse device ID %s\n", idstring);
		return 1;
	}

	if (!(driver = ifd_driver_for_id(&id))) {
		fprintf(stderr, "No driver for this device\n");
		return 1;
	}

	pid = ifd_spawn_handler(driver, device, -1);
	return (pid > 0)? 0 : 1;
}

/*
 * Show status of all readers
 */
int
mgr_status(int argc, char **argv)
{
	const ct_info_t *readers, *r;
	unsigned int	j;
	int		i, num, count = 0;
	char		*sepa;

	if (argc != 1)
		usage(1);
	if ((num = ct_status(&readers)) < 0) {
		fprintf(stderr, "Unable to get reader status\n");
		return 1;
	}

	for (i = 0, r = readers; i < num; i++, r++) {
		if (r->ct_pid == 0
		 || (kill(r->ct_pid, 0) < 0 && errno == ESRCH))
		 	continue;
		if (count == 0)
			printf("No.   Name                         Info\n"
			       "===================================================\n");
		printf(" %2d   %-29.29s", i, r->ct_name);

		sepa = "";
		if (r->ct_slots > 1) {
			printf("%s%d slots", sepa, r->ct_slots);
			sepa = ", ";
		}
		if (r->ct_display) {
			printf("%sdisplay", sepa);
			sepa = ", ";
		}
		if (r->ct_keypad) {
			printf("%skeypad", sepa);
			sepa = ", ";
		}
		for (j = 0; j < r->ct_slots; j++) {
			printf("%sslot%u: ", sepa, j);
			if (r->ct_card[j])
				printf("card present");
			else
				printf("empty");
			sepa = ", ";
		}
		printf("\n");
	}
	return 0;
}

/*
 * Configure a reader using info from the config file
 */
void
configure_reader(ifd_conf_node_t *cf)
{
	static unsigned int nreaders = 0;
	char		*device, *driver;

	if (ifd_conf_node_get_string(cf, "device", &device) < 0) {
		ct_error("no device specified in reader configuration");
		return;
	}

	if (ifd_conf_node_get_string(cf, "driver", &driver) < 0)
		driver = "auto";

	if (device == NULL && driver == NULL) {
		ct_error("neither device nor driver specified "
			  "in reader configuration");
		return;
	}

	ifd_spawn_handler(driver, device, nreaders++);
}

/*
 * Usage message
 */
void
usage(int exval)
{
	fprintf(stderr,
"usage: openct-control [-d] [-f configfile] command\n"
"  -d   enable debugging; repeat to increase verbosity\n"
"  -n   disable coldplugging\n"
"  -f   specify config file (default %s)\n"
"  -h   display this message\n"
"\nWhere command is one of:\n"
"init - initialize OpenCT\n"
"attach device ident - attach a hotplug device\n"
"status - display status of all readers present\n"
"shutdown - shutdown OpenCT\n", OPENCT_CONF_PATH
);
	exit(exval);
}
