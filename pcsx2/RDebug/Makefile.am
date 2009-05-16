pcsx2_dir = $(top_srcdir)/..
common_dir = $(pcsx2_dir)/common/include
third_dir = $(pcsx2_dir)/3rdparty
cdvd_dir = $(top_srcdir)/CDVD
x86_dir = $(top_srcdir)/x86
rdebug_dir = $(top_srcdir)/RDebug
debugtools_dir = $(top_srcdir)/DebugTools
ipu_dir = $(top_srcdir)/IPU
linux_dir = $(top_srcdir)/Linux

INCLUDES = -I$(top_srcdir)  -I$(cdvd_dir)  -I$(common_dir) -I$(third_dir)
noinst_LIBRARIES = libRDebug.a

libRDebug_a_SOURCES = \
deci2.cpp       deci2_dcmp.cpp  deci2_drfp.h    deci2_iloadp.h  deci2_ttyp.cpp \
deci2_dbgp.cpp  deci2_dcmp.h  deci2.h         deci2_netmp.cpp   deci2_ttyp.h \
deci2_dbgp.h  deci2_iloadp.cpp  deci2_netmp.h