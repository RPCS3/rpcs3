AUTOMAKE_OPTIONS = foreign
INCLUDES = $(shell pkg-config --cflags gtk+-2.0) -I@srcdir@/../   -I@srcdir@/../../common/include -I@srcdir@/../../3rdparty  $(shell wx-config --cppflags)

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

pcsx2_LDADD = ../libpcsx2.a ../IPU/libIPU.a ../IPU/mpeg2lib/libmpeg2IPU.a ../RDebug/libRDebug.a ../tinyxml/libtinyxml.a
pcsx2_LDADD += ../x86/libx86recomp.a ../x86/ix86/libix86.a
pcsx2_LDADD += ../DebugTools/libDebugTools.a