#pragma once
#include "Emu/RSX/GSRender.h"

class NullGSRender : public GSRender
{
public:
	NullGSRender(utils::serial* ar) noexcept;
	NullGSRender() noexcept : NullGSRender(nullptr) {}

	// There are no host surfaces to bind. Keeps the GRAPH engine on its fast path.
	bool is_framebuffer_bound() const override { return true; }
};
