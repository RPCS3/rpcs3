AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4 --install

pcsx2_dir = $(top_srcdir)/..
common_dir = $(pcsx2_dir)/common/include
third_dir = $(pcsx2_dir)/3rdparty
cdvd_dir = $(top_srcdir)/CDVD
x86_dir = $(top_srcdir)/x86
rdebug_dir = $(top_srcdir)/RDebug
debugtools_dir = $(top_srcdir)/DebugTools
ipu_dir = $(top_srcdir)/IPU
linux_dir = $(top_srcdir)/Linux

INCLUDES = -I$(x86_dir) -I$(cdvd_dir) -I$(common_dir) -I$(third_dir)
noinst_LIBRARIES = libpcsx2.a

libpcsx2_a_SOURCES = \
AlignedMalloc.cpp  HwRead.cpp       MTGS.cpp               R3000AInterpreter.cpp   Sio.cpp             VUflags.cpp \
Dump.cpp       HwWrite.cpp                            R3000AOpcodeTables.cpp  SourceLog.cpp       VUmicroMem.cpp \
Elfheader.cpp  Interpreter.cpp  Memory.cpp             R5900.cpp               Stats.cpp           VUops.cpp \
FPU.cpp        IopBios.cpp      MemoryCard.cpp         R5900OpcodeImpl.cpp     System.cpp          Vif.cpp \
COP0.cpp           FPU2.cpp       IopCounters.cpp  Misc.cpp               R5900OpcodeTables.cpp   ThreadTools.cpp     VifDma.cpp \
COP2.cpp           FiFo.cpp       IopDma.cpp       Patch.cpp              RecoverySystem.cpp      VU0.cpp             vssprintf.cpp \
Cache.cpp          GS.cpp         IopHw.cpp        PathUtils.cpp          SPR.cpp                 VU0micro.cpp        vtlb.cpp \
Gif.cpp        IopMem.cpp       Plugins.cpp            SaveState.cpp           VU0microInterp.cpp  xmlpatchloader.cpp \
Console.cpp                           IopSio2.cpp      PrecompiledHeader.cpp  Saveslots.cpp           VU1micro.cpp \
Counters.cpp       Hw.cpp         MMI.cpp          R3000A.cpp             Sif.cpp                 VU1microInterp.cpp
#Mdec.cpp HashTools.cpp


libpcsx2_a_SOURCES += \
ps2/Iop/IopHwRead.cpp ps2/Iop/IopHwWrite.cpp ps2/Iop/IopHw_Internal.h

libpcsx2_a_SOURCES += \
Common.h      HashMap.h      IopDma.h      MemoryCard.h   PrecompiledHeader.h  SafeArray.h  StringUtils.h  Vif.h \
Counters.h    HostGui.h      IopHw.h       MemoryTypes.h  R3000A.h             SamplProf.h  System.h       VifDma.h \
Hw.h           IopMem.h      Misc.h         R5900.h              SaveState.h  Threading.h    cheatscpp.h \
Dump.h        IopBios.h      IopSio2.h     NakedAsm.h     R5900Exceptions.h    Sif.h        VU.h           vtlb.h \
COP0.h        Elfheader.h   IopBios2.h     Mdec.h        Patch.h        R5900OpcodeTables.h  Sifcmd.h     VUflags.h \
Cache.h       Exceptions.h  IopCommon.h    MemcpyFast.h  Paths.h    Sio.h    Sio_internal.h    VUmicro.h \
GS.h          IopCounters.h  Memory.h      Plugins.h      SPR.h                Stats.h      VUops.h Tags.h
#RedtapeWindows.h

SUBDIRS = x86 . CDVD DebugTools IPU RDebug tinyxml Linux