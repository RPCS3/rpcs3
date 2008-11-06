INCLUDES = -I@srcdir@/../
noinst_LIBRARIES = libx86recomp.a

# have to add the sources instead of making a library since the linking is complicated
if X86_64
archfiles = ix86-64/iR5900-64.c   ix86-64/iR5900AritImm-64.c  ix86-64/iR5900Jump-64.c \
ix86-64/iR5900Move-64.c ix86-64/iR5900Shift-64.c ix86-64/iR5900Arit-64.c  ix86-64/iR5900Branch-64.c \
ix86-64/iR5900LoadStore-64.c  ix86-64/iR5900MultDiv-64.c ix86-64/iCore-64.cpp ix86-64/aR5900-64.S
else
archfiles = ix86-32/iR5900-32.c   ix86-32/iR5900AritImm.c  ix86-32/iR5900Jump.c \
ix86-32/iR5900Move.c ix86-32/iR5900Shift.c ix86-32/iR5900Arit.c  ix86-32/iR5900Branch.c \
ix86-32/iR5900LoadStore.c  ix86-32/iR5900MultDiv.c ix86-32/iCore-32.cpp ix86-32/aR5900-32.S
endif

libx86recomp_a_SOURCES = iCOP2.c  iCP0.c  iFPU.c  iHw.c  iMMI.c  iPsxHw.c  iPsxMem.c  \
ir5900tables.c  iVU0micro.c  iVU1micro.c  iVUmicro.c \
iCore.cpp  iGS.cpp  iR3000A.cpp  iR3000Atables.cpp  iVif.cpp  iVUzerorec.cpp \
fast_routines.S aR3000A.S aVUzerorec.S aVif.S $(archfiles)

libx86recomp_a_DEPENDENCIES = ix86/libix86.a

SUBDIRS = ix86
