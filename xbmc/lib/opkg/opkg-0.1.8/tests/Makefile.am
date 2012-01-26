AM_CFLAGS = $(ALL_CFLAGS) -Wall -g -O3 -I${top_srcdir}/libopkg

#noinst_PROGRAMS = opkg_hash_test opkg_extract_test
#noinst_PROGRAMS = libopkg_test opkg_active_list_test
noinst_PROGRAMS = libopkg_test

#opkg_hash_test_LDADD = $(top_builddir)/libbb/libbb.la $(top_builddir)/libopkg/libopkg.la
#opkg_hash_test_SOURCES = opkg_hash_test.c
#opkg_hash_test_CFLAGS = $(ALL_CFLAGS) -I$(top_srcdir)

#opkg_extract_test_LDADD = $(top_builddir)/libbb/libbb.la $(top_builddir)/libopkg/libopkg.la
#opkg_extract_test_SOURCES = opkg_extract_test.c
#opkg_extract_test_CFLAGS = $(ALL_CFLAGS) -I$(top_srcdir)

#opkg_active_list_test_LDADD = $(top_builddir)/libopkg/active_list.o
#opkg_active_list_test_SOURCES = opkg_active_list_test.c
#opkg_active_list_test_CFLAGS = $(ALL_CFLAGS) -I$(top_srcdir)

libopkg_test_LDADD = $(top_builddir)/libopkg/libopkg.la
libopkg_test_SOURCE = libopkg_test.c
libopkg_test_LDFLAGS = -static


