// NV47 Sync Objects
#pragma once

#include "context.h"

namespace rsx
{
	namespace nv406e
	{
		void set_reference(context* ctx, u32 reg, u32 arg);

		void semaphore_acquire(context* ctx, u32 reg, u32 arg);

		void semaphore_release(context* ctx, u32 reg, u32 arg);
	}
}
