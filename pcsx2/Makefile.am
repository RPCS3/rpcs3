AUTOMAKE_OPTIONS = foreign
INCLUDES = -I@srcdir@/x86/  -I@srcdir@/../common/include -I@srcdir@/../3rdparty
noinst_LIBRARIES = libpcsx2.a

libpcsx2_a_SOURCES = \
CDVD.cpp CDVDiso.cpp CDVDisodrv.cpp COP0.cpp COP2.cpp Cache.cpp CdRom.cpp Console.cpp HwRead.cpp HwWrite.cpp \
Counters.cpp Decode_XA.cpp Elfheader.cpp FPU.cpp FPU2.cpp FiFo.cpp Gif.cpp GS.cpp Hw.cpp Interpreter.cpp \
IopBios.cpp IopCounters.cpp IopDma.cpp IopHw.cpp IopMem.cpp IopSio2.cpp MMI.cpp MTGS.cpp \
Memory.cpp MemoryCard.cpp Misc.cpp Patch.cpp PathUtils.cpp Plugins.cpp PrecompiledHeader.cpp R3000A.cpp \
R3000AInterpreter.cpp R3000AOpcodeTables.cpp R5900.cpp R5900OpcodeImpl.cpp R5900OpcodeTables.cpp \
SPR.cpp SaveState.cpp Sif.cpp Sio.cpp SourceLog.cpp Stats.cpp System.cpp ThreadTools.cpp \
VU0.cpp VU0micro.cpp VU0microInterp.cpp VU1micro.cpp VU1microInterp.cpp VUflags.cpp VUmicroMem.cpp VUops.cpp \
Vif.cpp VifDma.cpp vssprintf.cpp vtlb.cpp xmlpatchloader.cpp  AlignedMalloc.cpp


libpcsx2_a_SOURCES += \
CDVD.h CDVDiso.h CDVDisodrv.h CDVDlib.h COP0.h Cache.h CdRom.h Common.h Counters.h Decode_XA.h EEregs.h \
Elfheader.h Exceptions.h GS.h Hw.h IopBios.h IopBios2.h IopCounters.h IopDma.h IopHw.h IopMem.h IopSio2.h Memcpyfast.h \
Memory.h MemoryCard.h Misc.h Patch.h Paths.h Plugins.h PrecompiledHeader.h PsxCommon.h R3000A.h R5900.h R5900OpcodeTables.h \
SPR.h SamplProf.h SaveState.h Sif.h Sifcmd.h Sio.h SafeArray.h Stats.h StringUtils.h System.h Threading.h \
VU.h VUflags.h VUmicro.h VUops.h Vif.h VifDma.h cheatscpp.h vtlb.h 

SUBDIRS = x86 . DebugTools IPU RDebug tinyxml Linux