
include $(top_srcdir)/Makefile.inc

lib_LTLIBRARIES = libcdc_videoengine.la

libcdc_videoengine_la_SOURCES = videoengine.c

libcdc_videoengine_la_CFLAGS = $(CFLAGS_CDCG)
LOCAL_INCLUDE = -I./ \
                -I./include \
                -I$(top_srcdir)/vdecoder/include \
                -I$(top_srcdir)/vdecoder \
                -I$(top_srcdir)/include \
                -I$(top_srcdir)/base/include \
                -I$(top_srcdir)/ve/include
						
libcdc_videoengine_la_CFLAGS += $(LOCAL_INCLUDE)


libcdc_videoengine_la_LDFLAGS = $(LDFLAGS_CDCG)

libcdc_videoengine_la_LIBADD = $(top_srcdir)/base/libcdc_base.la \
						$(top_srcdir)/ve/libcdc_ve.la \
						$(top_srcdir)/memory/libcdc_memory.la
LOCAL_LIB = -ldl
libcdc_videoengine_la_LDFLAGS += $(LOCAL_LIB)

#SUBDIRS = h264 mjpegplus mpeg4base mpeg4h263 mpeg4Vp6 mpeg4divx311 mpeg4normal \
#		  wmv3 avs h265 mjpeg mpeg2  vp8  vp9Hw

#h265soft_L  vp6soft  vp9soft  wmv12soft rv

