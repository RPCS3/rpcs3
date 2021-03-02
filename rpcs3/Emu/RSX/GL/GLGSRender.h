#pragma once
#include "Emu/RSX/GSRender.h"
#include "GLHelpers.h"
#include "GLTexture.h"
#include "GLTextureCache.h"
#include "GLRenderTargets.h"
#include "GLProgramBuffer.h"
#include "GLTextOut.h"
#include "GLOverlays.h"
#include "GLShaderInterpreter.h"

#include <optional>
#include <unordered_map>

#ifdef _WIN32
#pragma comment(lib, "opengl32.lib")
#endif

namespace gl
{
	using vertex_cache = rsx::vertex_cache::default_vertex_cache<rsx::vertex_cache::uploaded_range<GLenum>, GLenum>;
	using weak_vertex_cache = rsx::vertex_cache::weak_vertex_cache<GLenum>;
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

		volatile bool processed = false;
		volatile bool result = false;
		volatile bool received = false;

		void producer_wait()
		{
			while (!processed)
			{
				std::this_thread::yield();
			}

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
	};
}

class GLGSRender : public GSRender, public ::rsx::reports::ZCULL_control
{
private:
	GLFragmentProgram m_fragment_prog;
	GLVertexProgram m_vertex_prog;

	gl::sampler_state m_fs_sampler_states[rsx::limits::fragment_textures_count];         // Fragment textures
	gl::sampler_state m_fs_sampler_mirror_states[rsx::limits::fragment_textures_count];  // Alternate views of fragment textures with different format (e.g Depth vs Stencil for D24S8)
	gl::sampler_state m_vs_sampler_states[rsx::limits::vertex_textures_count];           // Vertex textures

	gl::glsl::program *m_program = nullptr;

	u32 m_interpreter_state = 0;
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

	// Identity buffer used to fix broken gl_VertexID on ATI stack
	std::unique_ptr<gl::buffer> m_identity_index_buffer;

	std::unique_ptr<gl::vertex_cache> m_vertex_cache;
	std::unique_ptr<gl::shader_cache> m_shaders_cache;

	GLint m_min_texbuffer_alignment = 256;
	GLint m_uniform_buffer_offset_align = 256;
	GLint m_max_texbuffer_size = 65536;

	bool manually_flush_ring_buffers = false;

	gl::text_writer m_text_printer;
	gl::ui_overlay_renderer m_ui_renderer;
	gl::video_out_calibration_pass m_video_output_pass;

	shared_mutex queue_guard;
	std::list<gl::work_item> work_queue;

	GLProgramBuffer m_prog_buffer;

	//buffer
	gl::fbo* m_draw_fbo = nullptr;
	std::list<gl::framebuffer_holder> m_framebuffer_cache;
	gl::fbo m_flip_fbo;
	std::unique_ptr<gl::texture> m_flip_tex_color;

	//vaos are mandatory for core profile
	gl::vao m_vao;

	shared_mutex m_sampler_mutex;
	atomic_t<bool> m_samplers_dirty = {true};
	std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::fragment_textures_count> fs_sampler_state = {};
	std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::vertex_textures_count> vs_sampler_state = {};
	std::unordered_map<GLenum, std::unique_ptr<gl::texture>> m_null_textures;
	std::vector<u8> m_scratch_buffer;

public:
	u64 get_cycles() final;
	GLGSRender();

private:

	gl::driver_state gl_state;

	// Return element to draw and in case of indexed draw index type and offset in index buffer
	gl::vertex_upload_info set_vertex_buffer();
	rsx::vertex_input_layout m_vertex_layout = {};

	void init_buffers(rsx::framebuffer_creation_context context, bool skip_reading = false);

	bool load_program();
	void load_program_env();
	void update_vertex_env(const gl::vertex_upload_info& upload_info);

	void update_draw_state();

	void load_texture_env();
	void bind_texture_env();

	gl::texture* get_present_source(gl::present_surface_info* info, const rsx::avconf& avconfig);

public:
	void set_viewport();
	void set_scissor(bool clip_viewport);

	gl::work_item& post_flush_request(u32 address, gl::texture_cache::thrashed_set& flush_data);

	bool scaled_image_from_memory(rsx::blit_src_info& src_info, rsx::blit_dst_info& dst_info, bool interpolate) override;

	void begin_occlusion_query(rsx::reports::occlusion_query_info* query) override;
	void end_occlusion_query(rsx::reports::occlusion_query_info* query) override;
	bool check_occlusion_query_status(rsx::reports::occlusion_query_info* query) override;
	void get_occlusion_query_result(rsx::reports::occlusion_query_info* query) override;
	void discard_occlusion_query(rsx::reports::occlusion_query_info* query) override;

protected:
	void clear_surface(u32 arg) override;
	void begin() override;
	void end() override;
	void emit_geometry(u32 sub_index) override;

	void on_init_thread() override;
	void on_exit() override;
	void flip(const rsx::display_flip_info_t& info) override;

	void do_local_task(rsx::FIFO_state state) override;

	bool on_access_violation(u32 address, bool is_writing) override;
	void on_invalidate_memory_range(const utils::address_range &range, rsx::invalidation_cause cause) override;
	void notify_tile_unbound(u32 tile) override;
	void on_semaphore_acquire_wait() override;

	std::array<std::vector<std::byte>, 4> copy_render_targets_to_memory() override;
	std::array<std::vector<std::byte>, 2> copy_depth_stencil_buffer_to_memory() override;
};
