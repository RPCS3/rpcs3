
// Pcsx2-specific Feature: Precompiled Header support!

#include "ptw32pch.h"

#ifndef PTW32_BUILD
#	pragma comment(lib, "w32pthreads.v4.lib")
#endif
