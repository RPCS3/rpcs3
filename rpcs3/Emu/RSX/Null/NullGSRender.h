#pragma once

#include "Emu/RSX/Core/RSXRenderBackend.h"
#include "Emu/RSX/GSFrameBase.h"

class NullGSRender : public rsx::render_backend
{
public:
	NullGSRender(rsx::context* ctx, rsx::display_output* output) noexcept;

	void on_init() override;
	void on_shutdown() override;

	// No host caches
	std::unique_ptr<rsx::context_resources> create_context_resources(rsx::context*) override { return std::make_unique<rsx::context_resources>(); }

	bool present_frame(const rsx::display_flip_info_t& info) override;

	// There are no host surfaces to bind. Keeps the GRAPH engine on its fast path.
	bool is_framebuffer_bound() const override { return true; }

	// Identifiers
	std::string logical_id() const override { return "Null"; }
	std::string configured_logical_id() const override;
};
