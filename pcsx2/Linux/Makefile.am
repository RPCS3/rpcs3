AUTOMAKE_OPTIONS = foreign

pcsx2_dir = $(top_srcdir)/..
common_dir = $(pcsx2_dir)/common/include
third_dir = $(pcsx2_dir)/3rdparty
cdvd_dir = $(top_srcdir)/CDVD
x86_dir = $(top_srcdir)/x86
rdebug_dir = $(top_srcdir)/RDebug
debugtools_dir = $(top_srcdir)/DebugTools
ipu_dir = $(top_srcdir)/IPU
linux_dir = $(top_srcdir)/Linux
gtk_includes= $(shell pkg-config --cflags gtk+-2.0)

INCLUDES = $(gtk_includes) -I$(top_srcdir)   -I$(cdvd_dir) -I$(common_dir) -I$(third_dir)

bin_PROGRAMS = pcsx2

# the application source, library search path, and link libraries
pcsx2_SOURCES =  \
interface.c support.c LnxMain.cpp LnxThreads.cpp LnxConsole.cpp LnxSysExec.cpp LnxMisc.cpp \
AboutDlg.cpp ConfigDlg.cpp DebugDlg.cpp AdvancedDlg.cpp CpuDlg.cpp HacksDlg.cpp McdDlgs.cpp Pref.cpp \
GtkGui.h Linux.h LnxMain.h ConfigDlg.h DebugDlg.h McdDlgs.h interface.h callbacks.h memzero.h support.h

pcsx2_LDFLAGS =

pcsx2_DEPENDENCIES = ../libpcsx2.a ../IPU/libIPU.a ../IPU/mpeg2lib/libmpeg2IPU.a ../RDebug/libRDebug.a ../tinyxml/libtinyxml.a
pcsx2_DEPENDENCIES += ../x86/libx86recomp.a ../x86/ix86/libix86.a
pcsx2_DEPENDENCIES += ../DebugTools/libDebugTools.a
pcsx2_DEPENDENCIES += ../CDVD/libps2_cdvd.a

pcsx2_LDADD = ../libpcsx2.a ../IPU/libIPU.a ../IPU/mpeg2lib/libmpeg2IPU.a ../RDebug/libRDebug.a ../tinyxml/libtinyxml.a
pcsx2_LDADD += ../x86/libx86recomp.a ../x86/ix86/libix86.a
pcsx2_LDADD += ../DebugTools/libDebugTools.a
pcsx2_LDADD += ../CDVD/libps2_cdvd.a