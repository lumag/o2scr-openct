AUTOMAKE_OPTIONS = foreign 1.10
ACLOCAL_AMFLAGS = -I aclocal

MAINTAINERCLEANFILES = \
	Makefile.in config.h.in config.h.in~ configure \
	install-sh ltmain.sh missing mkinstalldirs \
	compile depcomp config.log config.status \
	config.guess config.sub acinclude.m4 aclocal.m4 \
	packaged
EXTRA_DIST = svnignore

SUBDIRS = aclocal etc macos src doc

dist_noinst_SCRIPTS = bootstrap
dist_noinst_DATA = LGPL-2.1 TODO \
	solaris/Makefile.client solaris/Makefile.server solaris/README \
	solaris/checkinstall.in solaris/devices-openct-fabric.xml-dist \
	solaris/openct-fabric-dist solaris/openct-hotplug-dist \
	solaris/openct.conf-dist solaris/pkginfo.in solaris/postinstall \
	solaris/preremove solaris/proto
dist_doc_DATA = NEWS

# Allow detection of packaged tarball
dist-hook:
	echo > "$(distdir)/packaged"
