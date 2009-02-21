# Create a shared library libSPU2X
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libSPU2X.a
INCLUDES = -I@srcdir@/common  -I@srcdir@/3rdparty -I@srcdir@/Linux

libSPU2X_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0)
libSPU2X_a_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)

if X86_64
libSPU2X_a_CXXFLAGS += -fPIC
libSPU2X_a_CFLAGS += -fPIC
endif

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)

if DEBUGBUILD
preext=d
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libSPU2X
libSPU2X_SOURCES=
libSPU2X_DEPENDENCIES = libSPU2X.a 3rdparty/SoundTouch/libSoundTouch.a
libSPU2X_LDFLAGS= @SHARED_LDFLAGS@
libSPU2X_LDFLAGS+=-Wl,-soname,@SPU2X_SONAME@
libSPU2X_LDADD=$(llibSPU2X_a_OBJECTS) 3rdparty/SoundTouch/libSoundTouch.a

libSPU2X_a_SOURCES = ADSR.cpp DllInterface.cpp  Mixer.cpp RegTable.cpp  SaveStateSPU.cpp ConvertUTF.cpp \
Spu2.cpp Timestretcher.cpp  utf8.cpp Debug.cpp  Decoder.cpp  \
Dma.cpp Lowpass.cpp RegLog.cpp  Reverb.cpp SndOut.cpp Spu2replay.cpp  Wavedump_wav.cpp ReadInput.cpp

libSPU2X_a_SOURCES += BaseTypes.h  Debug.h  Dma.h  Lowpass.h  RegTable.h  SndOut.h ConvertUTF.h \
Spu2.h  Spu2replay.h  defs.h  regs.h  spdif.h  utf8.h
libSPU2X_a_SOURCES += Linux/Config.h Linux/Config.cpp Linux/Linux.h Linux/Alsa.cpp Linux/Alsa.h Linux/Dialogs.cpp Linux/Dialogs.h

SUBDIRS = 3rdparty/SoundTouch
