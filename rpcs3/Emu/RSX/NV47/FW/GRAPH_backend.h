#pragma once

#include "../HW/context.h"

namespace rsx
{
	// GRAPH backend class. Wraps RSX acceleration capabilities for the host.
	// TODO: Flesh this out.
	// TODO: Replace the virtuals with something faster
	class GRAPH_backend
	{
	public:
		// virtual void begin() = 0;
		// virtual void end() = 0;

		// Patch transform constants. Units are in 32x4 units
		virtual void patch_transform_constants(context* /*ctx*/, u32 /*index*/, u32 /*count*/) {};
	};
}
