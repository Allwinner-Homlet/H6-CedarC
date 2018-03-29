
include $(top_srcdir)/Makefile.inc

lib_LTLIBRARIES = libcdc_ve.la

libcdc_ve_la_SOURCES = veAw/veAw.c \
                       veVp9/veVp9.c \
                       veAdapter.c

libcdc_ve_la_CFLAGS = $(CFLAGS_CDCG)
LOCAL_INCLUDE = -I./include \
				-I$(top_srcdir)/base/include \
				-I$(top_srcdir)/include

libcdc_ve_la_CFLAGS += $(LOCAL_INCLUDE)

libcdc_ve_la_LDFLAGS = $(LDFLAGS_CDCG)

libcdc_ve_la_LIBADD = $(top_srcdir)/base/libcdc_base.la

#LOCAL_LIB = -lpthread
libcdc_ve_la_LDFLAGS += $(LOCAL_LIB)

