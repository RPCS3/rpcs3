pcsx2_dir = $(top_srcdir)/..
common_dir = $(pcsx2_dir)/common/include
third_dir = $(pcsx2_dir)/3rdparty
cdvd_dir = $(top_srcdir)/CDVD
x86_dir = $(top_srcdir)/x86
rdebug_dir = $(top_srcdir)/RDebug
debugtools_dir = $(top_srcdir)/DebugTools
ipu_dir = $(top_srcdir)/IPU
linux_dir = $(top_srcdir)/Linux

INCLUDES = -I$(top_srcdir) -I$(common_dir) -I$(third_dir)
noinst_LIBRARIES = libDebugTools.a

libDebugTools_a_SOURCES = \
DisR3000A.cpp DisR5900.cpp DisVU0Micro.cpp \
DisR3000asm.cpp  DisR5900asm.cpp  DisVU1Micro.cpp \
Debug.h  DisVUmicro.h DisASM.h  DisVUops.h
