INCLUDES = -I@srcdir@/../ -I@srcdir@/../x86 -I@srcdir@/../../common/include -I@srcdir@/../../3rdparty
noinst_LIBRARIES = libIPU.a

libIPU_a_SOURCES = IPU.cpp yuv2rgb.cpp coroutine.cpp \
IPU.h yuv2rgb.h coroutine.h \
acoroutine.S

SUBDIRS = mpeg2lib

#libIPU_a_LIBADD = mpeg2lib/libmpeg2IPU.a