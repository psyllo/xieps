# Build xieps_demo as libtool lib
lib_LTLIBRARIES = libxiepsdemo.la
libxiepsdemo_la_SOURCES = xieps_demo.h xieps_demo.c ../src/xi_data.h ../src/xi_data.c ../src/xi_error.c ../src/xi_error.h ../src/xi_enums.h ../src/xieps.h ../src/xieps.c
libxiepsdemo_la_CFLAGS = $(GLIB_CFLAGS)
libxiepsdemo_la_LIBS = ../src/libxieps.la $(GLIB_LIBS)
