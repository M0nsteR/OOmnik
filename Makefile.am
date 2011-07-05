## Process this file with automake to produce Makefile.in

SUBDIRS = src 

## what to clean

CLEANFILES = *~

## what additional things to distribute
EXTRA_DIST = oomnik.pc.in $(m4data_DATA)

## install docs
#docdir = $(datadir)/doc/$(PACKAGE)
#doc_DATA = SVNChangeLog ChangeLog ChangeLogOld NEWS README COPYING

## install check.pc
pcdatadir = $(libdir)/pkgconfig
pcdata_DATA = oomnik.pc

SVNChangeLog:
	svn log -v @abs_top_srcdir@ > $@
