# Process this file with automake to create Makefile.in
#
AUTOMAKE_OPTIONS = foreign 1.10
ACLOCAL_AMFLAGS = -I aclocal

MAINTAINERCLEANFILES = \
	Makefile.in config.h.in config.h.in~ configure \
	install-sh ltmain.sh missing mkinstalldirs \
	compile depcomp config.log config.status \
	config.guess config.sub acinclude.m4 aclocal.m4

DEPCLEANFILES = config.log configure

SUBDIRS = aclocal etc macos src doc

doc_DATA = NEWS

dist_noinst_DATA = LGPL-2.1 NEWS TODO bootstrap depcomp \
	solaris/Makefile.client solaris/Makefile.server solaris/README \
	solaris/checkinstall.in solaris/devices-openct-fabric.xml-dist \
	solaris/openct-fabric-dist solaris/openct-hotplug-dist \
	solaris/openct.conf-dist solaris/pkginfo.in solaris/postinstall \
	solaris/preremove solaris/proto

