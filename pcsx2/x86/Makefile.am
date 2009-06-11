pcsx2_dir = $(top_srcdir)/..
common_dir = $(pcsx2_dir)/common/include
third_dir = $(pcsx2_dir)/3rdparty
cdvd_dir = $(top_srcdir)/CDVD
x86_dir = $(top_srcdir)/x86
rdebug_dir = $(top_srcdir)/RDebug
debugtools_dir = $(top_srcdir)/DebugTools
ipu_dir = $(top_srcdir)/IPU
linux_dir = $(top_srcdir)/Linux

INCLUDES = -I$(top_srcdir) -I$(common_dir) -I$(ipu_dir) -I$(cdvd_dir) -I$(third_dir)
noinst_LIBRARIES = libx86recomp.a

# have to add the sources instead of making a library since the linking is complicated

archfiles = ix86-32/iR5900-32.cpp   ix86-32/iR5900AritImm.cpp  ix86-32/iR5900Jump.cpp ix86-32/iR5900Move.cpp \
ix86-32/iR5900Shift.cpp ix86-32/iR5900Arit.cpp  ix86-32/iR5900Branch.cpp ix86-32/iR5900LoadStore.cpp  \
ix86-32/iR5900MultDiv.cpp ix86-32/iCore-32.cpp ix86-32/aR5900-32.S ix86-32/iR5900Templates.cpp ix86-32/recVTLB.cpp

libx86recomp_a_SOURCES = \
BaseblockEx.cpp    iCOP2.cpp  iFPUd.cpp    iR3000A.cpp        iVU0micro.cpp  ir5900tables.cpp  sVU_Micro.cpp \
iCore.cpp  iMMI.cpp     iR3000Atables.cpp  iVU1micro.cpp  microVU.cpp       sVU_Upper.cpp \
iCOP0.cpp          iFPU.cpp   iPsxMem.cpp  iR5900Misc.cpp     iVif.cpp       sVU_Lower.cpp     sVU_zerorec.cpp \
aR3000A.S  aVUzerorec.S  aVif.S  fast_routines.S $(archfiles) 

libx86recomp_a_SOURCES += \
microVU_Alloc.inl    microVU_Compile.inl  microVU_Flags.inl  microVU_Lower.inl  microVU_Tables.inl \
microVU_Analyze.inl  microVU_Execute.inl  microVU_Log.inl    microVU_Misc.inl   microVU_Upper.inl

libx86recomp_a_SOURCES += \
BaseblockEx.h  iFPU.h     iR5900.h         iR5900Branch.h     iR5900Move.h     microVU.h       sVU_Debug.h \
iCOP0.h        iMMI.h     iR5900Arit.h     iR5900Jump.h       iR5900MultDiv.h  microVU_IR.h    sVU_Micro.h \
iCore.h        iR3000A.h  iR5900AritImm.h  iR5900LoadStore.h  iR5900Shift.h    microVU_Misc.h  sVU_zerorec.h


libx86recomp_a_DEPENDENCIES = ix86/libix86.a

SUBDIRS = ix86