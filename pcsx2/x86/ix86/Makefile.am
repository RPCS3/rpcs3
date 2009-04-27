INCLUDES = -I@srcdir@/.. -I@srcdir@/../../   -I@srcdir@/../../../common/include -I@srcdir@/../../../3rdparty -I/implement  -I/implement/xmm $(shell wx-config --cppflags)
noinst_LIBRARIES = libix86.a

libix86_a_SOURCES = \
ix86.cpp ix86_cpudetect.cpp ix86_fpu.cpp ix86_jmp.cpp ix86_tools.cpp ix86_3dnow.cpp \
ix86_legacy.cpp ix86_legacy_sse.cpp ix86_simd.cpp \
ix86_internal.h ix86_legacy_instructions.h ix86_macros.h ix86_sse_helpers.h ix86.h ix86_legacy_internal.h \
ix86_instructions.h ix86_legacy_types.h ix86_types.h \
bittest.h  dwshift.h  group1.h  group2.h  group3.h  incdec.h  jmpcall.h  movs.h  test.h \
movqss.h arithmetic.h shufflepack.h basehelpers.h comparisons.h moremovs.h xchg.h