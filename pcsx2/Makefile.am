AUTOMAKE_OPTIONS = foreign
INCLUDES = -I@srcdir@/x86/
noinst_LIBRARIES = libpcsx2.a

libpcsx2_a_SOURCES = \
CdRom.c       Decode_XA.h    Mdec.h          PsxBios.h         R3000A.c  Vif.h \
CdRom.h       EEregs.h       PsxCommon.h       R3000A.h  VU0.c \
CDVD.c        Elfheader.c    Memory.c        PsxCounters.c     R5900.c   VU0.h \
CDVD.h        Elfheader.h    Memory.h        PsxCounters.h     R5900.h   VU0micro.c \
CDVDiso.c     FiFo.c         Misc.c          PsxDma.c          Sif.c     VU1micro.c \
CDVDisodrv.c  FPU2.cpp       PsxDma.h          Sifcmd.h  VUflags.c \
CDVDisodrv.h  FPU.c          MMI.c           Sif.h     VUflags.h \
CDVDiso.h     GS.cpp         Patch.c         Sio.c     VU.h \
CDVDlib.h     GS.h           Patch.h         PsxHw.c           Sio.h     VUmicro.h \
Common.h      Hw.c           Plugins.c       PsxHw.h           SPR.c     VUops.c \
COP0.c        Hw.h           Plugins.h       PsxInterpreter.c  SPR.h     VUops.h \
COP0.h        Interpreter.c  PS2Edefs.h      PsxMem.c          System.h \
Counters.c    InterTables.c  PS2Etypes.h     PsxMem.h          Vif.c \
Counters.h    InterTables.h  PsxBios2.h      PsxSio2.c         VifDma.c \
Decode_XA.c   Mdec.c         PsxBios.c       PsxSio2.h         VifDma.h Cache.c \
xmlpatchloader.cpp

if RECBUILD
recdir = x86
else
recdir=
endif

SUBDIRS = $(recdir) . DebugTools IPU RDebug tinyxml Linux