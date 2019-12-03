#include "stdafx.h"
// Defines STB_IMAGE_IMPLEMENTATION *once* for stb_image.h includes (Should this be placed somewhere else?)
#define STB_IMAGE_IMPLEMENTATION

// Sneak in truetype as well.
#define STB_TRUETYPE_IMPLEMENTATION

// This header generates lots of errors, so we ignore those (not rpcs3 code)
#ifdef _MSC_VER
#pragma warning(push, 0)
#include <stb_image.h>
#include <stb_truetype.h>
#pragma warning(pop)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <stb_image.h>
#include <stb_truetype.h>
#pragma GCC diagnostic pop
#endif
