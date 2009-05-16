pcsx2_dir = $(top_srcdir)/..
common_dir = $(pcsx2_dir)/common/include
third_dir = $(pcsx2_dir)/3rdparty
cdvd_dir = $(top_srcdir)/CDVD
x86_dir = $(top_srcdir)/x86
rdebug_dir = $(top_srcdir)/RDebug
debugtools_dir = $(top_srcdir)/DebugTools
ipu_dir = $(top_srcdir)/IPU
linux_dir = $(top_srcdir)/Linux

INCLUDES = -I$(ipu_dir) -I$(top_srcdir) -I$(common_dir) -I$(third_dir)
noinst_LIBRARIES = libmpeg2IPU.a

libmpeg2IPU_a_SOURCES = Idct.cpp Mpeg.cpp Mpeg.h Vlc.h