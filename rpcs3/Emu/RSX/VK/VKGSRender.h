#pragma once
#include "Emu/RSX/GSRender.h"
#include "Emu/Cell/timers.hpp"

#include "upscalers/upscaling.h"

#include "vkutils/descriptors.h"
#include "vkutils/data_heap.h"
#include "vkutils/instance.hpp"
#include "vkutils/sync.h"
#include "vkutils/swapchain.hpp"

#include "VKGSRenderTypes.hpp"
#include "VKTextureCache.h"
#include "VKRenderTargets.h"
#include "VKFormats.h"
#include "VKTextOut.h"
#include "VKOverlays.h"
#include "VKProgramBuffer.h"
#include "VKFramebuffer.h"
#include "VKShaderInterpreter.h"
#include "VKQueryPool.h"
#include "../GCM.h"
#include "util/asm.hpp"

#include <thread>
#include <optional>

using namespace vk::vmm_allocation_pool_; // clang workaround.
using namespace vk::upscaling_flags_;     // ditto

class VKGSRender : public GSRender, public ::rsx::reports::ZCULL_control
{
private:
	enum
	{
		VK_HEAP_CHECK_TEXTURE_UPLOAD_STORAGE = 0x1,
		VK_HEAP_CHECK_VERTEX_STORAGE = 0x2,
		VK_HEAP_CHECK_VERTEX_ENV_STORAGE = 0x4,
		VK_HEAP_CHECK_FRAGMENT_ENV_STORAGE = 0x8,
		VK_HEAP_CHECK_TEXTURE_ENV_STORAGE = 0x10,
		VK_HEAP_CHECK_VERTEX_LAYOUT_STORAGE = 0x20,
		VK_HEAP_CHECK_TRANSFORM_CONSTANTS_STORAGE = 0x40,
		VK_HEAP_CHECK_FRAGMENT_CONSTANTS_STORAGE = 0x80,

		VK_HEAP_CHECK_MAX_ENUM = VK_HEAP_CHECK_FRAGMENT_CONSTANTS_STORAGE,
		VK_HEAP_CHECK_ALL = 0xFF,
	};

	enum frame_context_state : u32
	{
		dirty = 1
	};

	enum flush_queue_state : u32
	{
		ok = 0,
		flushing = 1,
		deadlock = 2
	};

private:
	const VKFragmentProgram *m_fragment_prog = nullptr;
	const VKVertexProgram *m_vertex_prog = nullptr;
	vk::glsl::program *m_program = nullptr;
	vk::pipeline_props m_pipeline_properties;

	vk::texture_cache m_texture_cache;
	vk::surface_cache m_rtts;

	std::unique_ptr<vk::buffer> null_buffer;
	std::unique_ptr<vk::buffer_view> null_buffer_view;

	std::unique_ptr<vk::text_writer> m_text_writer;
	std::unique_ptr<vk::upscaler> m_upscaler;
	bool m_use_fsr_upscaling{false};

	std::unique_ptr<vk::buffer> m_cond_render_buffer;
	u64 m_cond_render_sync_tag = 0;

	shared_mutex m_sampler_mutex;
	atomic_t<bool> m_samplers_dirty = { true };
	std::unique_ptr<vk::sampler> m_stencil_mirror_sampler;
	std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::fragment_textures_count> fs_sampler_state = {};
	std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::vertex_textures_count> vs_sampler_state = {};
	std::array<vk::sampler*, rsx::limits::fragment_textures_count> fs_sampler_handles{};
	std::array<vk::sampler*, rsx::limits::vertex_textures_count> vs_sampler_handles{};

	std::unique_ptr<vk::buffer_view> m_persistent_attribute_storage;
	std::unique_ptr<vk::buffer_view> m_volatile_attribute_storage;
	std::unique_ptr<vk::buffer_view> m_vertex_layout_storage;

public:
	//vk::fbo draw_fbo;
	std::unique_ptr<vk::vertex_cache> m_vertex_cache;
	std::unique_ptr<vk::shader_cache> m_shaders_cache;

private:
	std::unique_ptr<vk::program_cache> m_prog_buffer;

	std::unique_ptr<vk::swapchain_base> m_swapchain;
	vk::instance m_instance;
	vk::render_device *m_device;

	//Vulkan internals
	std::unique_ptr<vk::query_pool_manager> m_occlusion_query_manager;
	bool m_occlusion_query_active = false;
	rsx::reports::occlusion_query_info *m_active_query_info = nullptr;
	std::vector<vk::occlusion_data> m_occlusion_map;

	shared_mutex m_secondary_cb_guard;
	vk::command_pool m_secondary_command_buffer_pool;
	vk::command_buffer_chain<VK_MAX_ASYNC_CB_COUNT> m_secondary_cb_list;

	vk::command_pool m_command_buffer_pool;
	vk::command_buffer_chain<VK_MAX_ASYNC_CB_COUNT> m_primary_cb_list;
	vk::command_buffer_chunk* m_current_command_buffer = nullptr;
	VkSemaphore m_dangling_semaphore_signal = VK_NULL_HANDLE;

	volatile vk::host_data_t* m_host_data_ptr = nullptr;
	std::unique_ptr<vk::buffer> m_host_object_data;

	VkDescriptorSetLayout descriptor_layouts;
	VkPipelineLayout pipeline_layout;

	vk::framebuffer_holder* m_draw_fbo = nullptr;

	sizeu m_swapchain_dims{};
	bool swapchain_unavailable = false;
	bool should_reinitialize_swapchain = false;

	u64 m_last_heap_sync_time = 0;
	u32 m_texbuffer_view_size = 0;

	vk::data_heap m_attrib_ring_info;                  // Vertex data
	vk::data_heap m_fragment_constants_ring_info;      // Fragment program constants
	vk::data_heap m_transform_constants_ring_info;     // Transform program constants
	vk::data_heap m_fragment_env_ring_info;            // Fragment environment params
	vk::data_heap m_vertex_env_ring_info;              // Vertex environment params
	vk::data_heap m_fragment_texture_params_ring_info; // Fragment texture params
	vk::data_heap m_vertex_layout_ring_info;           // Vertex layout structure
	vk::data_heap m_index_buffer_ring_info;            // Index data
	vk::data_heap m_texture_upload_buffer_ring_info;   // Texture upload heap
	vk::data_heap m_raster_env_ring_info;              // Raster control such as polygon and line stipple

	vk::data_heap m_fragment_instructions_buffer;
	vk::data_heap m_vertex_instructions_buffer;

	VkDescriptorBufferInfo m_vertex_env_buffer_info;
	VkDescriptorBufferInfo m_fragment_env_buffer_info;
	VkDescriptorBufferInfo m_vertex_layout_stream_info;
	VkDescriptorBufferInfo m_vertex_constants_buffer_info;
	VkDescriptorBufferInfo m_fragment_constants_buffer_info;
	VkDescriptorBufferInfo m_fragment_texture_params_buffer_info;
	VkDescriptorBufferInfo m_raster_env_buffer_info;

	VkDescriptorBufferInfo m_vertex_instructions_buffer_info;
	VkDescriptorBufferInfo m_fragment_instructions_buffer_info;

	std::array<vk::frame_context_t, VK_MAX_ASYNC_FRAMES> frame_context_storage;
	//Temp frame context to use if the real frame queue is overburdened. Only used for storage
	vk::frame_context_t m_aux_frame_context;

	u32 m_current_queue_index = 0;
	vk::frame_context_t* m_current_frame = nullptr;
	std::deque<vk::frame_context_t*> m_queued_frames;

	VkViewport m_viewport{};
	VkRect2D m_scissor{};

	std::vector<u8> m_draw_buffers;

	shared_mutex m_flush_queue_mutex;
	vk::flush_request_task m_flush_requests;

	ullong m_last_cond_render_eval_hint = 0;

	// Offloader thread deadlock recovery
	rsx::atomic_bitmask_t<flush_queue_state> m_queue_status;
	utils::address_range m_offloader_fault_range;
	rsx::invalidation_cause m_offloader_fault_cause;

	vk::draw_call_t m_current_draw = {};
	u64 m_current_renderpass_key = 0;
	VkRenderPass m_cached_renderpass = VK_NULL_HANDLE;
	std::vector<vk::image*> m_fbo_images;

	//Vertex layout
	rsx::vertex_input_layout m_vertex_layout;

	vk::shader_interpreter m_shader_interpreter;
	u32 m_interpreter_state;

#if defined(HAVE_X11) && defined(HAVE_VULKAN)
	Display *m_display_handle = nullptr;
#endif

public:
	u64 get_cycles() final;
	VKGSRender();
	~VKGSRender() override;

private:
	void prepare_rtts(rsx::framebuffer_creation_context context);

	void close_and_submit_command_buffer(
		vk::fence* fence = nullptr,
		VkSemaphore wait_semaphore = VK_NULL_HANDLE,
		VkSemaphore signal_semaphore = VK_NULL_HANDLE,
		VkPipelineStageFlags pipeline_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

	void flush_command_queue(bool hard_sync = false, bool do_not_switch = false);
	void queue_swap_request();
	void frame_context_cleanup(vk::frame_context_t *ctx);
	void advance_queued_frames();
	void present(vk::frame_context_t *ctx);
	void reinitialize_swapchain();

	vk::viewable_image* get_present_source(vk::present_surface_info* info, const rsx::avconf& avconfig);

	void begin_render_pass();
	void close_render_pass();
	VkRenderPass get_render_pass();

	void update_draw_state();

	void check_heap_status(u32 flags = VK_HEAP_CHECK_ALL);
	void check_present_status();

	void check_descriptors();
	VkDescriptorSet allocate_descriptor_set();

	vk::vertex_upload_info upload_vertex_data();

	bool load_program();
	void load_program_env();
	void update_vertex_env(u32 id, const vk::vertex_upload_info& vertex_info);

	void load_texture_env();
	bool bind_texture_env();
	bool bind_interpreter_texture_env();

public:
	void init_buffers(rsx::framebuffer_creation_context context, bool skip_reading = false);
	void set_viewport();
	void set_scissor(bool clip_viewport);
	void bind_viewport();

	void sync_hint(rsx::FIFO_hint hint, rsx::reports::sync_hint_payload_t payload) override;
	bool release_GCM_label(u32 address, u32 data) override;

	void begin_occlusion_query(rsx::reports::occlusion_query_info* query) override;
	void end_occlusion_query(rsx::reports::occlusion_query_info* query) override;
	bool check_occlusion_query_status(rsx::reports::occlusion_query_info* query) override;
	void get_occlusion_query_result(rsx::reports::occlusion_query_info* query) override;
	void discard_occlusion_query(rsx::reports::occlusion_query_info* query) override;

	// External callback in case we need to suddenly submit a commandlist unexpectedly, e.g in a violation handler
	void emergency_query_cleanup(vk::command_buffer* commands);

	// External callback to handle out of video memory problems
	bool on_vram_exhausted(rsx::problem_severity severity);

	// Conditional rendering
	void begin_conditional_rendering(const std::vector<rsx::reports::occlusion_query_info*>& sources) override;
	void end_conditional_rendering() override;

	// Host sync object
	inline std::pair<volatile vk::host_data_t*, VkBuffer> map_host_object_data() { return { m_host_data_ptr, m_host_object_data->value }; }

protected:
	void clear_surface(u32 mask) override;
	void begin() override;
	void end() override;
	void emit_geometry(u32 sub_index) override;

	void on_init_thread() override;
	void on_exit() override;
	void flip(const rsx::display_flip_info_t& info) override;

	void renderctl(u32 request_code, void* args) override;

	void do_local_task(rsx::FIFO_state state) override;
	bool scaled_image_from_memory(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate) override;
	void notify_tile_unbound(u32 tile) override;

	bool on_access_violation(u32 address, bool is_writing) override;
	void on_invalidate_memory_range(const utils::address_range &range, rsx::invalidation_cause cause) override;
	void on_semaphore_acquire_wait() override;
};
