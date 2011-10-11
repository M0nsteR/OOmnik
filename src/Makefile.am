## Process this file with automake to produce Makefile.in

CLEANFILES = *~

lib_LTLIBRARIES = liboomnik.la

liboomnik_la_SOURCES = oomnik.h oomnik.c\
                    oomindmap.h oomindmap.c\
                    ooconcept.h ooconcept.c\
                    ootopic.h ootopic.c\
                    ooconstraint.h ooconstraint.c\
                    oocode.h oocode.c\
                    oocodesystem.h oocodesystem.c\
                    oocomplex.h oocomplex.c\
                    oodecoder.h oodecoder.c\
                    ooagenda.h ooagenda.c\
                    ooaccumulator.h ooaccumulator.c\
                    oosegmentizer.h oosegmentizer.c\
                    oocache.h oocache.c\
                    ooconcunit.h ooconcunit.c\
                    ooarray.h ooarray.c\
                    oolist.h oolist.c\
                    oodict.h oodict.c

include_HEADERS =  oomnik.h ooconfig.h\
                    oomindmap.h\
                    ooconcept.h\
                    ootopic.h\
                    ooconstraint.h\
                    oocode.h\
                    oocodesystem.h\
                    oocomplex.h\
                    oodecoder.h\
                    ooaccumulator.h\
                    ooagenda.h\
                    oosegmentizer.h\
                    oocache.h\
                    ooarray.h\
                    oolist.h\
                    oodict.h\
                    ooconcunit.h

bin_PROGRAMS = oomnik
oomnik_SOURCES = main.c

oomnik_LDADD = liboomnik.la


