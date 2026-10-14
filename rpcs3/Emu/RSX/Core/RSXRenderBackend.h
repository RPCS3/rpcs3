#pragma once

#include <util/types.hpp>
#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "Emu/RSX/Common/TextureUtils.h"
#include "Emu/RSX/Common/profiling_timer.hpp"
#include "Emu/system_config_types.h"
#include "Emu/RSX/Common/texture_cache_types.h"
#include "Emu/RSX/Utils/rsx_utils.h"
#include "Emu/RSX/Core/RSXDisplay.h"
#include "Emu/RSX/RSXFIFO.h"
#include "Emu/RSX/RSXZCULL.h"

// Forward-declared to keep display.h (and with it windows.h) out of RSXThread.h
class GSFrameBase;
using draw_context_t = void*;

namespace rsx
{
	struct context;
	struct framebuffer_layout;
	class display_output;
	class RSXDMAWriter;

	struct backend_configuration
	{
		bool supports_multidraw;                 // Draw call batching
		bool supports_hw_a2c;                    // Alpha to coverage
		bool supports_hw_a2c_1spp;               // Alpha to coverage at 1 sample per pixel
		bool supports_hw_renormalization;        // Should be true on NV hardware which matches PS3 texture renormalization behaviour
		bool supports_hw_msaa;                   // MSAA support
		bool supports_hw_a2one;                  // Alpha to one
		bool supports_hw_conditional_render;     // Conditional render
		bool supports_hw_instanced_rendering;    // Instanced draws
		bool supports_passthrough_dma;           // DMA passthrough
		bool supports_asynchronous_compute;      // Async compute
		bool supports_host_gpu_labels;           // Advanced host synchronization
		bool supports_normalized_barycentrics;   // Basically all GPUs except NVIDIA have properly normalized barycentrics
		bool supports_last_provoking_vertex;     // Flat shading using RSX's last-vertex convention
		bool supports_programmable_blending;     // Can handle programmable blending requests
		bool supports_extended_depth_range;      // Depth values above 1 can be stored and tested. Float depth targets are emulated otherwise.
		bool supports_framebufferless_rendering; // Can rasterize without any attachments bound (e.g for occlusion queries)
	};

	// Host renderer interface. Translates common state into work for the host accelerator.
	// Backends are just graphics accelerator plugins. A backend never calls back up into rsx::thread.
	class render_backend
	{
	public:
		// The backend renders for the given context and presents to the given display output (the host window).
		render_backend(context* ctx, display_output* display);
		virtual ~render_backend();

		render_backend(const render_backend&) = delete;
		render_backend& operator=(const render_backend&) = delete;

		// Host capabilities. Filled in by the backend during initialization; zero until then.
		const backend_configuration& config() const { return m_config; }

		// Host DMA writer used for host GPU labels, if the backend has one. The RSX thread updates it.
		RSXDMAWriter* host_dma_writer() const { return m_host_dma_ctrl.get(); }

		// Backend lifecycle management.
		virtual void on_init() {}
		virtual void on_shutdown() {}

		// Framebuffer. Returns false if the host could not build the framebuffer; the core then treats the configuration as invalid.
		virtual bool on_framebuffer_bound(const framebuffer_layout& /*layout*/) { return true; }
		virtual bool is_framebuffer_bound() const { return false; }

		// Draw
		virtual void begin_draw() {}
		virtual bool end_draw() { return false; }
		// Clear the bound surfaces. The core has decoded the clear into draw_state.clear.
		virtual void clear() {}
		virtual void emit_geometry(u32 /*sub_index*/) {}

		// Fill the sampler descriptors of the texture units the current programs reference. Runs before the core resolves the programs.
		// The invalidated_programs field receives the program state bits that host-side changes invalidated (e.g. a texture's format class).
		virtual bool resolve_sampler_state(rsx::flags32_t& /*invalidated_programs*/) { return false; }

		// Filled by resolve_sampler_state. The descriptors are a cache of the texture units and are lost with the backend.
		const std::array<std::unique_ptr<sampled_image_descriptor_base>, limits::fragment_textures_count>& fragment_sampler_descriptors() const { return m_fs_sampler_descriptors; }
		const std::array<std::unique_ptr<sampled_image_descriptor_base>, limits::vertex_textures_count>& vertex_sampler_descriptors() const { return m_vs_sampler_descriptors; }

		// Present. Returns true if a frame was presented, in which case post_present() runs after the core's flip bookkeeping.
		virtual bool present_frame(const display_flip_info_t& info) = 0;
		virtual void post_present() {}

		// Host part of conditional rendering. The core updates the context's conditional render state after these.
		virtual void on_begin_conditional_rendering(const std::vector<reports::occlusion_query_info*>& /*sources*/) {}
		virtual void on_end_conditional_rendering() {}

		// Memory
		virtual bool on_access_violation(u32 /*address*/, bool /*is_writing*/) { return false; }
		virtual void on_invalidate_memory_range(const utils::address_range32& /*range*/, invalidation_cause /*cause*/) {}
		virtual void notify_tile_unbound(u32 /*tile*/) {}
		virtual void write_barrier(u32 /*memory_address*/, u32 /*memory_range*/) {}

		// Load an image from memory with optional scaling and rotation.
		// Returns false to tell the HW decoder to perform the operation on the CPU as a fallback when the operation cannot be safely accelerated.
		virtual bool scaled_image_from_memory(const blit_src_info& /*src_info*/, const blit_dst_info& /*dst_info*/, bool /*interpolate*/) { return false; }

		// Sync
		virtual bool release_GCM_label(u32 /*type*/, u32 /*address*/, u32 /*value*/) { return false; }
		virtual void on_semaphore_acquire_wait() {}

		// Host part of a sync hint. The core forwards the hint to ZCULL first.
		virtual void on_sync_hint(FIFO::interrupt_hint /*hint*/, reports::sync_hint_payload_t /*payload*/) {}

		// Backend housekeeping at the start of a local task invocation. Returns false to skip the rest of the task, e.g. when re-entered while the backend is flushing.
		virtual bool on_local_task(FIFO::state /*state*/) { return true; }

		// Backend-specific control requests. Returns false if the request code is not handled.
		virtual bool on_renderctl(u32 /*request_code*/, void* /*args*/) { return false; }

		// ZCULL. Host occlusion queries; nullptr reports everything as visible.
		virtual reports::occlusion_query_provider* occlusion_queries() { return nullptr; }

		// GRAPH. Patch transform constants. Units are in 32x4 units
		virtual void patch_transform_constants(context* /*ctx*/, u32 /*index*/, u32 /*count*/) {}

		// Inspection
		virtual std::pair<std::string, std::string> get_programs() const { return std::make_pair("", ""); }
		virtual bool is_current_program_interpreted() const { return false; }

	protected:
		// Generic draw context on the host window, made current on the calling thread. GL creates its own context instead.
		void create_frame_context();
		void destroy_frame_context();

		backend_configuration m_config{};

		// The context this backend renders for. Set by the core.
		context* m_ctx = nullptr;

		// Presentation. The window belongs to the RSX thread (display_output); the draw context and the swap interval are the backend's.
		GSFrameBase* m_frame = nullptr;
		draw_context_t m_context = nullptr;
		vsync_mode m_vsync_mode{};

		// The RSX thread's timer. The core starts it and the backend continues its intervals.
		rsx::profiling_timer& m_profiler;
		std::unique_ptr<RSXDMAWriter> m_host_dma_ctrl;

		std::array<std::unique_ptr<sampled_image_descriptor_base>, limits::fragment_textures_count> m_fs_sampler_descriptors = {};
		std::array<std::unique_ptr<sampled_image_descriptor_base>, limits::vertex_textures_count> m_vs_sampler_descriptors = {};
	};
}
