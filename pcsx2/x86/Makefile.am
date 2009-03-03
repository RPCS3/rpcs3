INCLUDES = -I@srcdir@/../   -I@srcdir@/../../common/include -I@srcdir@/../IPU/ -I@srcdir@/../../3rdparty
noinst_LIBRARIES = libx86recomp.a

# have to add the sources instead of making a library since the linking is complicated

archfiles = ix86-32/iR5900-32.cpp   ix86-32/iR5900AritImm.cpp  ix86-32/iR5900Jump.cpp \
ix86-32/iR5900Move.cpp ix86-32/iR5900Shift.cpp ix86-32/iR5900Arit.cpp  ix86-32/iR5900Branch.cpp \
ix86-32/iR5900LoadStore.cpp  ix86-32/iR5900MultDiv.cpp ix86-32/iCore-32.cpp ix86-32/aR5900-32.S \
ix86-32/iR5900Templates.cpp ix86-32/recVTLB.cpp

libx86recomp_a_SOURCES = \
BaseblockEx.cpp iCOP0.cpp iCOP2.cpp iCore.cpp iFPU.cpp iFPUd.cpp iGS.cpp iHw.cpp iIPU.cpp iMMI.cpp iPsxHw.cpp iPsxMem.cpp \
iR3000A.cpp iR3000Atables.cpp iR5900CoissuedLoadStore.cpp iR5900Misc.cpp iVU0micro.cpp iVU1micro.cpp iVUmicro.cpp \
iVUmicroLower.cpp iVUmicroUpper.cpp iVUzerorec.cpp iVif.cpp ir5900tables.cpp  fast_routines.S aR3000A.S aVUzerorec.S \
aVif.S $(archfiles)

libx86recomp_a_SOURCES += \
BaseblockEx.h iCOP0.h iCore.h iFPU.h iMMI.h iR3000A.h iR5900.h iR5900Arit.h iR5900AritImm.h iR5900Branch.h iR5900Jump.h \
iR5900LoadStore.h iR5900Move.h iR5900MultDiv.h iR5900Shift.h iVUmicro.h iVUops.h iVUzerorec.h 

#ifdef PCSX2_MICROVU
libx86recomp_a_SOURCES += \
microVU.cpp microVU_Alloc.cpp microVU_Compile.cpp microVU_Lower.cpp microVU_Misc.cpp microVU_Upper.cpp aMicroVU.S \
microVU.h microVU_Alloc.h microVU_Misc.h microVU_Tables.h
#endif

libx86recomp_a_DEPENDENCIES = ix86/libix86.a

SUBDIRS = ix86