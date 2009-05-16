pcsx2_dir = $(top_srcdir)/..
common_dir = $(pcsx2_dir)/common/include
third_dir = $(pcsx2_dir)/3rdparty
cdvd_dir = $(top_srcdir)/CDVD
x86_dir = $(top_srcdir)/x86
rdebug_dir = $(top_srcdir)/RDebug
debugtools_dir = $(top_srcdir)/DebugTools
ipu_dir = $(top_srcdir)/IPU
linux_dir = $(top_srcdir)/Linux

INCLUDES = -I$(top_srcdir) -I$(x86_dir) -I$(cdvd_dir) -I$(common_dir) -I$(third_dir) -I/implement -I/implement/xmm
noinst_LIBRARIES = libix86.a

libix86_a_SOURCES = \
ix86.cpp ix86_cpudetect.cpp ix86_fpu.cpp ix86_jmp.cpp ix86_tools.cpp ix86_3dnow.cpp \
ix86_legacy.cpp ix86_legacy_sse.cpp ix86_simd.cpp \
ix86_internal.h ix86_legacy_instructions.h ix86_macros.h ix86_sse_helpers.h ix86.h ix86_legacy_internal.h \
ix86_instructions.h ix86_legacy_types.h ix86_types.h \
bittest.h  dwshift.h  group1.h  group2.h  group3.h  incdec.h  jmpcall.h  movs.h  test.h \
movqss.h arithmetic.h shufflepack.h basehelpers.h comparisons.h moremovs.h xchg.h