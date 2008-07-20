#pragma once

// for memory leaks
#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#endif // _DEBUG