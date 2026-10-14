#pragma once

#include "Emu/RSX/Core/RSXRenderBackend.h"
#include "Emu/RSX/GSFrameBase.h"

class NullGSRender : public rsx::render_backend
{
public:
	NullGSRender(rsx::context* ctx, rsx::display_output* output) noexcept;

	void on_init() override;
	void on_shutdown() override;

	bool present_frame(const rsx::display_flip_info_t& info) override;

	// There are no host surfaces to bind. Keeps the GRAPH engine on its fast path.
	bool is_framebuffer_bound() const override { return true; }
};
