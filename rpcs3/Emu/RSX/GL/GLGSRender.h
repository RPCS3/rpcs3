#pragma once
#include "Emu/RSX/Core/RSXRenderBackend.h"
#include "Emu/RSX/GSFrameBase.h"
#include "Emu/RSX/RSXThread.h"
#include "GLTextureCache.h"
#include "GLRenderTargets.h"
#include "GLProgramBuffer.h"
#include "GLOverlays.h"
#include "GLShaderInterpreter.h"
#include "Emu/RSX/rsx_cache.h"
#include "util/asm.hpp"

#include <optional>
#include <unordered_map>
#include <thread>

#include "glutils/ring_buffer.h"
#include "upscalers/upscaling.h"

#ifdef _MSC_VER
#pragma comment(lib, "opengl32.lib")
#endif

using namespace gl::upscaling_flags_;

namespace gl
{
	using vertex_cache = rsx::vertex_cache::default_vertex_cache<rsx::vertex_cache::uploaded_range>;
	using weak_vertex_cache = rsx::vertex_cache::weak_vertex_cache;
	using null_vertex_cache = vertex_cache;

	using shader_cache = rsx::shaders_cache<void*, GLProgramBuffer>;

	struct vertex_upload_info
	{
		u32 vertex_draw_count;
		u32 allocated_vertex_count;
		u32 first_vertex;
		u32 vertex_index_base;
		u32 vertex_index_offset;
		u32 persistent_mapping_offset;
		u32 volatile_mapping_offset;
		std::optional<std::tuple<GLenum, u32> > index_info;
	};

	struct work_item
	{
		u32  address_to_flush = 0;
		gl::texture_cache::thrashed_set section_data;

		atomic_t<bool> processed = false;
		volatile bool result = false;
		atomic_t<bool> received = false;

		void producer_wait()
		{
			utils::spin_wait(processed, [](auto v)
			{
				return v;
			});

			received = true;
		}
	};

	struct present_surface_info
	{
		u32 address;
		u32 format;
		u32 width;
		u32 height;
		u32 pitch;
		u8  eye;
	};

	struct context_resources final : rsx::context_resources
	{
		std::unique_ptr<gl::vertex_cache> vertex_cache;
	};
}

class GLGSRender : public rsx::render_backend, public ::rsx::reports::occlusion_query_provider
{
	gl::sampler_state m_fs_sampler_states[rsx::limits::fragment_textures_count];         // Fragment textures
	gl::sampler_state m_fs_sampler_mirror_states[rsx::limits::fragment_textures_count];  // Alternate views of fragment textures with different format (e.g Depth vs Stencil for D24S8)
	gl::sampler_state m_vs_sampler_states[rsx::limits::vertex_textures_count];           // Vertex textures

	gl::glsl::program *m_program = nullptr;
	gl::glsl::program* m_prev_program = nullptr;
	const GLFragmentProgram *m_fragment_prog = nullptr;
	const GLVertexProgram *m_vertex_prog = nullptr;
	rsx::bitmask_t<rsx::draw_state_t::group> m_stale_constants;                           // Constant buffers whose host copy is out of date.

	rsx::bitmask_t<rsx::draw_state_t::group> m_interpreter_state;
	gl::shader_interpreter m_shader_interpreter;

	gl_render_targets m_rtts;

	gl::texture_cache m_gl_texture_cache;

	gl::buffer_view m_persistent_stream_view;
	gl::buffer_view m_volatile_stream_view;
	std::unique_ptr<gl::texture> m_gl_persistent_stream_buffer;
	std::unique_ptr<gl::texture> m_gl_volatile_stream_buffer;

	std::unique_ptr<gl::ring_buffer> m_attrib_ring_buffer;
	std::unique_ptr<gl::ring_buffer> m_fragment_constants_buffer;
	std::unique_ptr<gl::ring_buffer> m_transform_constants_buffer;
	std::unique_ptr<gl::ring_buffer> m_fragment_env_buffer;
	std::unique_ptr<gl::ring_buffer> m_vertex_env_buffer;
	std::unique_ptr<gl::ring_buffer> m_texture_parameters_buffer;
	std::unique_ptr<gl::ring_buffer> m_vertex_layout_buffer;
	std::unique_ptr<gl::ring_buffer> m_index_ring_buffer;
	std::unique_ptr<gl::ring_buffer> m_vertex_instructions_buffer;
	std::unique_ptr<gl::ring_buffer> m_fragment_instructions_buffer;
	std::unique_ptr<gl::ring_buffer> m_raster_env_ring_buffer;
	std::unique_ptr<gl::ring_buffer> m_instancing_ring_buffer;

	// Identity buffer used to fix broken gl_VertexID on ATI stack
	std::unique_ptr<gl::buffer> m_identity_index_buffer;

	// Used for hot-patching
	std::unique_ptr<gl::ring_buffer> m_scratch_ring_buffer;

	std::unique_ptr<gl::shader_cache> m_shaders_cache;

	GLint m_min_texbuffer_alignment = 256;
	GLint m_uniform_buffer_offset_align = 256;
	GLint m_min_ssbo_alignment = 256;
	GLint m_max_texbuffer_size = 65536;

	bool manually_flush_ring_buffers = false;

	gl::ui_overlay_renderer m_ui_renderer;
	gl::video_out_calibration_pass m_video_output_pass;

	shared_mutex queue_guard;
	std::list<gl::work_item> work_queue;

	GLProgramBuffer m_prog_buffer;

	// Draw Buffers
	gl::fbo* m_draw_fbo = nullptr;
	std::list<gl::framebuffer_holder> m_framebuffer_cache;
	std::unique_ptr<gl::texture> m_flip_tex_color[2];

	// Present
	gl::fbo m_sshot_fbo;
	std::unique_ptr<gl::texture> m_sshot_tex;
	std::unique_ptr<gl::upscaler> m_upscaler;
	output_scaling_mode m_output_scaling = output_scaling_mode::bilinear;
	int m_last_client_width = 0;  // Window size of the last presented frame
	int m_last_client_height = 0;

	// VAOs are mandatory for core profile
	gl::vao m_vao;

	shared_mutex m_sampler_mutex;
	atomic_t<bool> m_samplers_dirty = {true};
	std::unordered_map<GLenum, std::unique_ptr<gl::viewable_image>> m_null_textures;
	rsx::simple_array<u8> m_scratch_buffer;

	// Occlusion query type, can be SAMPLES_PASSED or ANY_SAMPLES_PASSED
	GLenum m_occlusion_type = GL_ANY_SAMPLES_PASSED;
	std::array<GLuint, rsx::reports::occlusion_query_count> m_occlusion_query_handles{}; // Host query per ZCULL query slot

	// Host context for GPU-driven work
	std::unique_ptr<gl::buffer> m_host_gpu_context_data;
	std::unique_ptr<gl::scratch_ring_buffer> m_enqueued_host_write_buffer;

	// State tracking
	gl::driver_state gl_state;

public:
	GLGSRender(rsx::context* ctx, rsx::display_output* output) noexcept;
	virtual ~GLGSRender();

private:
	// Return element to draw and in case of indexed draw index type and offset in index buffer
	gl::vertex_upload_info set_vertex_buffer();
	rsx::vertex_input_layout m_vertex_layout = {};

	bool load_program();
	void load_program_env();
	void update_vertex_env(const gl::vertex_upload_info& upload_info);
	void upload_transform_constants(const rsx::io_buffer& buffer);

	void update_draw_state();

	rsx::flags32_t load_texture_env();
	void bind_texture_env();

	gl::texture* get_present_source(gl::present_surface_info* info, const rsx::avconf& avconfig);

	void update_swap_interval();

	static gl::context_resources* res(rsx::context* ctx) { return static_cast<gl::context_resources*>(ctx->resources.get()); }
	gl::context_resources* res() const { return res(m_ctx); }

public:
	void set_viewport();
	void set_scissor(bool force = false);

	gl::work_item& post_flush_request(u32 address, gl::texture_cache::thrashed_set& flush_data);

	// NV3089
	bool scaled_image_from_memory(const rsx::blit_src_info& src_info, const rsx::blit_dst_info& dst_info, bool interpolate) override;

	// Sync
	void write_barrier(u32 address, u32 range) override;

	// ZCULL
	void begin_occlusion_query(rsx::reports::occlusion_query_info* query) override;
	void end_occlusion_query(rsx::reports::occlusion_query_info* query) override;
	bool check_occlusion_query_status(rsx::reports::occlusion_query_info* query) override;
	void get_occlusion_query_result(rsx::reports::occlusion_query_info* query) override;
	void discard_occlusion_query(rsx::reports::occlusion_query_info* query) override;
	rsx::reports::occlusion_query_provider* occlusion_queries() override { return this; }

	// DMA
	bool release_GCM_label(u32 type, u32 address, u32 data) override;
	void enqueue_host_context_write(u32 offset, u32 size, const void* data);
	void on_guest_texture_read();

	// GRAPH backend
	void patch_transform_constants(rsx::context* ctx, u32 index, u32 count) override;

	// Misc
	bool is_current_program_interpreted() const override;

protected:
	void clear() override;
	void begin_draw() override;
	bool resolve_sampler_state(rsx::flags32_t& invalidated_programs) override;
	bool end_draw() override;
	void emit_geometry(u32 sub_index) override;

	bool on_framebuffer_bound(const rsx::framebuffer_layout& layout) override;
	bool is_framebuffer_bound() const override;

	void on_init() override;
	void on_shutdown() override;
	std::unique_ptr<rsx::context_resources> create_context_resources(rsx::context* ctx) override;
	void on_bind_context(rsx::context* ctx) override;
	void on_unbind_context(rsx::context* ctx) override;
	bool present_frame(const rsx::display_flip_info_t& info) override;
	void post_present() override;

	bool on_local_task(rsx::FIFO::state state) override;

	bool on_access_violation(rsx::context* ctx, u32 address, bool is_writing) override;
	void on_invalidate_memory_range(rsx::context* ctx, const utils::address_range32 &range, rsx::invalidation_cause cause) override;
	void notify_tile_unbound(u32 tile) override;
	void on_semaphore_acquire_wait() override;
};
