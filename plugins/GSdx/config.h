#pragma once

//#define ENABLE_VTUNE

#define ENABLE_JIT_RASTERIZER

//#define ENABLE_DYNAMIC_CRC_HACK

#define ENABLE_UPSCALE_HACKS // Hacks intended to fix upscaling / rendering glitches in HW renderers

//#define DISABLE_HW_TEXTURE_CACHE // Slow but fixes a lot of bugs

//#define DISABLE_CRC_HACKS // Disable all game specific hacks

#if defined(DISABLE_HW_TEXTURE_CACHE) && !defined(DISABLE_CRC_HACKS)
	#define DISABLE_CRC_HACKS
#endif

//#define DISABLE_BITMASKING

//#define DISABLE_COLCLAMP

//#define DISABLE_DATE
