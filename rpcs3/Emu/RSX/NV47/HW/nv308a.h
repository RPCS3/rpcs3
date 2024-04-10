// NV47 Format Conversion
#pragma once

#include "context.h"

namespace rsx
{
	namespace nv308a
	{
		struct color
		{
			static void impl(context* ctx, u32 reg, u32 arg);
		};
	}
}
