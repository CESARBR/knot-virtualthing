AM_MAKEFLAGS = --no-print-directory

ACLOCAL_AMFLAGS = -I m4

modules_ldadd = @ELL_LIBS@
modules_cflags = @ELL_CFLAGS@

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)
AM_LDFLAGS = $(BUILD_LDFLAGS)

bin_PROGRAMS = src/thingd

src_thingd_SOURCES = src/main.c

src_thingd_LDADD = $(modules_ldadd) -lm
src_thingd_LDFLAGS = $(AM_LDFLAGS)
src_thingd_CFLAGS = $(AM_CFLAGS) $(modules_cflags)

MAINTAINERCLEANFILES = Makefile.in \
        aclocal.m4 configure config.h.in config.sub config.guess \
        ltmain.sh depcomp compile missing install-sh

clean-local:
	$(RM) -r src/thingd
