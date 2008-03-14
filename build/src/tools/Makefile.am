MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = openct-tool
sbin_PROGRAMS = openct-control
man1_MANS = openct-tool.1

openct_tool_SOURCES = openct-tool.c
openct_tool_LDADD = $(top_srcdir)/src/ct/libopenct.la
openct_tool_CFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/src/include

openct_control_SOURCES = openct-control.c
openct_control_LDADD = $(top_srcdir)/src/ifd/libifd.la $(top_srcdir)/src/ct/libopenct.la
openct_control_CFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/src/include
