#include "stdafx.h"
// Defines STB_IMAGE_IMPLEMENTATION *once* for stb_image.h includes (Should this be placed somewhere else?)
#define STB_IMAGE_IMPLEMENTATION

// This header generates lots of errors, so we ignore those (not rpcs3 code)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <stb_image.h>
#pragma clang diagnostic pop

#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#include <stb_image.h>
#pragma GCC diagnostic pop

#elif defined(_MSC_VER)
// TODO Turn off warnings for MSVC. Using the normal push warning levels simply
// creates a new warning about warnings being supressed (ie fuck msvc)
// #pragma warning( push, 4 )
#include <stb_image.h>
// #pragma warning( pop )

#else
#include <stb_image.h>
#endif
