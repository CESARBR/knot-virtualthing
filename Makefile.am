AM_MAKEFLAGS = --no-print-directory

ACLOCAL_AMFLAGS = -I m4

modules_ldadd = @ELL_LIBS@ @JSON_LIBS@ @RABBITMQ_LIBS@ \
		@KNOTPROTO_LIBS@ @MODBUS_LIBS@
modules_cflags = @ELL_CFLAGS@ @JSON_CFLAGS@ @RABBITMQ_CFLAGS@ \
		@KNOTPROTO_CFLAGS@ @MODBUS_CFLAGS@

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)
AM_LDFLAGS = $(BUILD_LDFLAGS)

bin_PROGRAMS = src/thingd

src_thingd_SOURCES = src/main.c \
			src/sm.c \
			src/mq.c src/mq.h \
			src/parser.c src/parser.h \
			src/cloud.c src/cloud.h \
			src/device.c src/device.h \
			src/storage.c src/storage.h \
			src/modbus-interface.c src/modbus-interface.h \
			src/modbus-tcp.c src/modbus-rtu.c \
			src/modbus-driver.h \
			src/settings.c src/settings.h \
			src/knot-config.c src/knot-config.h \
			src/poll.c src/poll.h

src_thingd_LDADD = $(modules_ldadd) -lm
src_thingd_LDFLAGS = $(AM_LDFLAGS)
src_thingd_CFLAGS = $(AM_CFLAGS) $(modules_cflags)

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp compile missing install-sh

TESTS = tests/sm_tests tests/device_tests
check_PROGRAMS = $(TESTS)

tests_cflags = $(modules_cflags) @CHECK_CFLAGS@
tests_ldadd = $(modules_ldadd) @CHECK_LIBS@ -lm

tests_sm_tests_SOURCES = tests/st-machine-test.c \
			src/sm.c src/sm-pvt.h \
			tests/mocks/fake-device.c tests/mocks/fake-device.h

tests_sm_tests_CFLAGS = $(tests_cflags)
tests_sm_tests_LDADD = $(tests_ldadd)

tests_device_tests_SOURCES = tests/device-tests.c \
			src/device.c src/device.h src/device-pvt.h \
			src/mq.c src/mq.h \
			src/parser.c src/parser.h \
			src/cloud.c src/cloud.h \
			src/storage.c src/storage.h \
			src/sm.c src/sm.h \
			src/modbus-interface.c src/modbus-interface.h \
			src/modbus-tcp.c src/modbus-rtu.c \
			src/modbus-driver.h \
			src/settings.c src/settings.h \
			src/knot_config.c src/knot_config.h \
			src/poll.c src/poll.h

tests_device_tests_CFLAGS = $(tests_cflags)
tests_device_tests_LDADD = $(tests_ldadd)

clean-local:
	$(RM) -r src/thingd
