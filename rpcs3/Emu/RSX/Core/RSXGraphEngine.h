#pragma once

#include <util/types.hpp>
#include <array>
#include <memory>
#include "Emu/RSX/Core/RSXFrameBuffer.h"
#include "Emu/RSX/Utils/rsx_utils.h"
#include "Utilities/geometry.h"

namespace rsx
{
	struct context;
	class sampled_image_descriptor_base;

	// The GRAPH functional unit. It is a per-context component of the renderer core.
	class graph_engine
	{
	public:
		void init(context* ctx) { m_ctx = ctx; }

		std::array<u32, 4> get_color_surface_addresses() const;
		u32 get_zeta_surface_address() const;

		// Resolves the framebuffer for a draw or a clear and hands a changed layout to the backend.
		// On return, rtt_config_valid tells whether the framebuffer can be rendered to.
		void bind_framebuffer(rsx::framebuffer_creation_context context);

		// Re-evaluates the framebuffer write state after a surface option register write
		void on_framebuffer_options_changed(u32 opt);

		bool requires_depth_range_emulation() const;

		// Runs shader prefetch and resolves pipeline status flags
		void analyse_current_rsx_pipeline();

		// Gets the current vertex program and associated state. Can invalidate the bound progam.
		void get_current_vertex_program(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::vertex_textures_count>& sampler_descriptors);

		// Gets current fragment program and associated fragment state. Can invalidate the bound program.
		void get_current_fragment_program(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::fragment_textures_count>& sampler_descriptors);

		// Forces a fragment ucode reload if a DMA write overlaps the bound program. Returns true if it did.
		bool invalidate_fragment_program(u32 dst_dma, u32 dst_offset, u32 size);

	private:
		context* m_ctx = nullptr;

		void get_framebuffer_layout(rsx::framebuffer_creation_context context, framebuffer_layout& layout);

		// Resolves the unscaled scissor region into draw_state.scissor. Can invalidate the framebuffer.
		void resolve_scissor(bool clip_viewport);

		// The committed layout can change the fragment program's MRT output count
		void on_framebuffer_layout_updated();

		// Prefetch and analyze the currently active fragment program ucode
		void prefetch_fragment_program();

		// Prefetch and analyze the currently active vertex program ucode
		void prefetch_vertex_program();

		// Update fragment program export configuration. Can invalidate the current program.
		rsx::flags32_t get_fragment_program_export_config();
	};
}
