#pragma once
#include "Emu/RSX/GSRender.h"
#include "VKHelpers.h"
#include "VKTextureCache.h"
#include "VKRenderTargets.h"
#include "VKFormats.h"
#include "VKTextOut.h"
#include "VKOverlays.h"
#include "VKProgramBuffer.h"
#include "VKFramebuffer.h"
#include "../GCM.h"

#include <thread>
#include <atomic>
#include <optional>

namespace vk
{
	using vertex_cache = rsx::vertex_cache::default_vertex_cache<rsx::vertex_cache::uploaded_range<VkFormat>, VkFormat>;
	using weak_vertex_cache = rsx::vertex_cache::weak_vertex_cache<VkFormat>;
	using null_vertex_cache = vertex_cache;

	using shader_cache = rsx::shaders_cache<vk::pipeline_props, VKProgramBuffer>;

	struct vertex_upload_info
	{
		VkPrimitiveTopology primitive;
		u32 vertex_draw_count;
		u32 allocated_vertex_count;
		u32 first_vertex;
		u32 vertex_index_base;
		u32 vertex_index_offset;
		u32 persistent_window_offset;
		u32 volatile_window_offset;
		std::optional<std::tuple<VkDeviceSize, VkIndexType>> index_info;
	};
}

// Initial heap allocation values. The heaps are growable and will automatically increase in size to accomodate demands
#define VK_ATTRIB_RING_BUFFER_SIZE_M 64
#define VK_TEXTURE_UPLOAD_RING_BUFFER_SIZE_M 64
#define VK_UBO_RING_BUFFER_SIZE_M 16
#define VK_TRANSFORM_CONSTANTS_BUFFER_SIZE_M 16
#define VK_FRAGMENT_CONSTANTS_BUFFER_SIZE_M 16
#define VK_INDEX_RING_BUFFER_SIZE_M 16

#define VK_MAX_ASYNC_CB_COUNT 256
#define VK_MAX_ASYNC_FRAMES 2

using rsx::flags32_t;
extern u64 get_system_time();

namespace vk
{
	struct command_buffer_chunk: public vk::command_buffer
	{
		vk::fence* submit_fence = nullptr;
		VkDevice m_device = VK_NULL_HANDLE;

		std::atomic_bool pending = { false };
		u64 eid_tag = 0;
		u64 reset_id = 0;
		shared_mutex guard_mutex;

		command_buffer_chunk() = default;

		void init_fence(VkDevice dev)
		{
			m_device = dev;
			submit_fence = new vk::fence(dev);
		}

		void destroy()
		{
			vk::command_buffer::destroy();
			delete submit_fence;
		}

		void tag()
		{
			eid_tag = vk::get_event_id();
		}

		void reset()
		{
			if (pending)
				poke();

			if (pending)
				wait(FRAME_PRESENT_TIMEOUT);

			++reset_id;
			CHECK_RESULT(vkResetCommandBuffer(commands, 0));
		}

		bool poke()
		{
			reader_lock lock(guard_mutex);

			if (!pending)
				return true;

			if (!submit_fence->flushed)
				return false;

			if (vkGetFenceStatus(m_device, submit_fence->handle) == VK_SUCCESS)
			{
				lock.upgrade();

				if (pending)
				{
					vk::reset_fence(submit_fence);
					vk::on_event_completed(eid_tag);

					pending = false;
					eid_tag = 0;
				}
			}

			return !pending;
		}

		VkResult wait(u64 timeout = 0ull)
		{
			reader_lock lock(guard_mutex);

			if (!pending)
				return VK_SUCCESS;

			const auto ret = vk::wait_for_fence(submit_fence, timeout);

			lock.upgrade();

			if (pending)
			{
				vk::reset_fence(submit_fence);
				vk::on_event_completed(eid_tag);

				pending = false;
				eid_tag = 0;
			}

			return ret;
		}

		void flush()
		{
			reader_lock lock(guard_mutex);

			if (!pending)
				return;

			submit_fence->wait_flush();
		}
	};

	struct occlusion_data
	{
		rsx::simple_array<u32> indices;
		command_buffer_chunk* command_buffer_to_wait = nullptr;
		u64 command_buffer_sync_id = 0;

		bool is_current(command_buffer_chunk* cmd) const
		{
			return (command_buffer_to_wait == cmd && command_buffer_sync_id == cmd->reset_id);
		}

		void set_sync_command_buffer(command_buffer_chunk* cmd)
		{
			command_buffer_to_wait = cmd;
			command_buffer_sync_id = cmd->reset_id;
		}

		void sync()
		{
			if (command_buffer_to_wait->reset_id == command_buffer_sync_id)
			{
				// Allocation stack is FIFO and very long so no need to actually wait for fence signal
				command_buffer_to_wait->flush();
			}
		}
	};

	struct frame_context_t
	{
		VkSemaphore acquire_signal_semaphore = VK_NULL_HANDLE;
		VkSemaphore present_wait_semaphore = VK_NULL_HANDLE;
		VkDescriptorSet descriptor_set = VK_NULL_HANDLE;

		vk::descriptor_pool descriptor_pool;
		u32 used_descriptors = 0;

		flags32_t flags = 0;

		std::vector<std::unique_ptr<vk::buffer_view>> buffer_views_to_clean;

		u32 present_image = UINT32_MAX;
		command_buffer_chunk* swap_command_buffer = nullptr;

		//Heap pointers
		s64 attrib_heap_ptr = 0;
		s64 vtx_env_heap_ptr = 0;
		s64 frag_env_heap_ptr = 0;
		s64 frag_const_heap_ptr = 0;
		s64 vtx_const_heap_ptr = 0;
		s64 vtx_layout_heap_ptr = 0;
		s64 frag_texparam_heap_ptr = 0;
		s64 index_heap_ptr = 0;
		s64 texture_upload_heap_ptr = 0;

		u64 last_frame_sync_time = 0;

		//Copy shareable information
		void grab_resources(frame_context_t &other)
		{
			present_wait_semaphore = other.present_wait_semaphore;
			acquire_signal_semaphore = other.acquire_signal_semaphore;
			descriptor_set = other.descriptor_set;
			descriptor_pool = other.descriptor_pool;
			used_descriptors = other.used_descriptors;
			flags = other.flags;

			attrib_heap_ptr = other.attrib_heap_ptr;
			vtx_env_heap_ptr = other.vtx_env_heap_ptr;
			frag_env_heap_ptr = other.frag_env_heap_ptr;
			vtx_layout_heap_ptr = other.vtx_layout_heap_ptr;
			frag_texparam_heap_ptr = other.frag_texparam_heap_ptr;
			frag_const_heap_ptr = other.frag_const_heap_ptr;
			vtx_const_heap_ptr = other.vtx_const_heap_ptr;
			index_heap_ptr = other.index_heap_ptr;
			texture_upload_heap_ptr = other.texture_upload_heap_ptr;
		}

		//Exchange storage (non-copyable)
		void swap_storage(frame_context_t &other)
		{
			std::swap(buffer_views_to_clean, other.buffer_views_to_clean);
		}

		void tag_frame_end(s64 attrib_loc, s64 vtxenv_loc, s64 fragenv_loc, s64 vtxlayout_loc, s64 fragtex_loc, s64 fragconst_loc,s64 vtxconst_loc, s64 index_loc, s64 texture_loc)
		{
			attrib_heap_ptr = attrib_loc;
			vtx_env_heap_ptr = vtxenv_loc;
			frag_env_heap_ptr = fragenv_loc;
			vtx_layout_heap_ptr = vtxlayout_loc;
			frag_texparam_heap_ptr = fragtex_loc;
			frag_const_heap_ptr = fragconst_loc;
			vtx_const_heap_ptr = vtxconst_loc;
			index_heap_ptr = index_loc;
			texture_upload_heap_ptr = texture_loc;

			last_frame_sync_time = get_system_time();
		}

		void reset_heap_ptrs()
		{
			last_frame_sync_time = 0;
		}
	};

	struct flush_request_task
	{
		atomic_t<bool> pending_state{ false };  //Flush request status; true if rsx::thread is yet to service this request
		atomic_t<int> num_waiters{ 0 };  //Number of threads waiting for this request to be serviced
		bool hard_sync = false;

		flush_request_task() = default;

		void post(bool _hard_sync)
		{
			hard_sync = (hard_sync || _hard_sync);
			pending_state = true;
			num_waiters++;
		}

		void remove_one()
		{
			num_waiters--;
		}

		void clear_pending_flag()
		{
			hard_sync = false;
			pending_state.store(false);
		}

		bool pending() const
		{
			return pending_state.load();
		}

		void consumer_wait() const
		{
			while (num_waiters.load() != 0)
			{
				_mm_pause();
			}
		}

		void producer_wait() const
		{
			while (pending_state.load())
			{
				std::this_thread::yield();
			}
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
	VKFragmentProgram m_fragment_prog;
	VKVertexProgram m_vertex_prog;
	vk::glsl::program *m_program;

	vk::texture_cache m_texture_cache;
	rsx::vk_render_targets m_rtts;

	std::unique_ptr<vk::buffer> null_buffer;
	std::unique_ptr<vk::buffer_view> null_buffer_view;

	std::unique_ptr<vk::text_writer> m_text_writer;
	std::unique_ptr<vk::depth_convert_pass> m_depth_converter;
	std::unique_ptr<vk::ui_overlay_renderer> m_ui_renderer;
	std::unique_ptr<vk::attachment_clear_pass> m_attachment_clear_pass;
	std::unique_ptr<vk::video_out_calibration_pass> m_video_output_pass;

	std::unique_ptr<vk::buffer> m_cond_render_buffer;
	u64 m_cond_render_sync_tag = 0;

	shared_mutex m_sampler_mutex;
	u64 surface_store_tag = 0;
	std::atomic_bool m_samplers_dirty = { true };
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
	std::unique_ptr<VKProgramBuffer> m_prog_buffer;

	std::unique_ptr<vk::swapchain_base> m_swapchain;
	vk::context m_thread_context;
	vk::render_device *m_device;

	//Vulkan internals
	vk::command_pool m_command_buffer_pool;
	vk::occlusion_query_pool m_occlusion_query_pool;
	bool m_occlusion_query_active = false;
	rsx::reports::occlusion_query_info *m_active_query_info = nullptr;
	std::vector<vk::occlusion_data> m_occlusion_map;

	shared_mutex m_secondary_cb_guard;
	vk::command_pool m_secondary_command_buffer_pool;
	vk::command_buffer m_secondary_command_buffer;  //command buffer used for setup operations

	u32 m_current_cb_index = 0;
	std::array<vk::command_buffer_chunk, VK_MAX_ASYNC_CB_COUNT> m_primary_cb_list;
	vk::command_buffer_chunk* m_current_command_buffer = nullptr;

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

	VkDescriptorBufferInfo m_vertex_env_buffer_info;
	VkDescriptorBufferInfo m_fragment_env_buffer_info;
	VkDescriptorBufferInfo m_vertex_layout_stream_info;
	VkDescriptorBufferInfo m_vertex_constants_buffer_info;
	VkDescriptorBufferInfo m_fragment_constants_buffer_info;
	VkDescriptorBufferInfo m_fragment_texture_params_buffer_info;

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

	// Offloader thread deadlock recovery
	rsx::atomic_bitmask_t<flush_queue_state> m_queue_status;
	utils::address_range m_offloader_fault_range;
	rsx::invalidation_cause m_offloader_fault_cause;

	u32 m_current_subdraw_id = 0;
	u64 m_current_renderpass_key = 0;
	VkRenderPass m_cached_renderpass = VK_NULL_HANDLE;
	std::vector<vk::image*> m_fbo_images;

	//Vertex layout
	rsx::vertex_input_layout m_vertex_layout;

#if defined(HAVE_X11) && defined(HAVE_VULKAN)
	Display *m_display_handle = nullptr;
#endif

public:
	u64 get_cycles() final;
	VKGSRender();
	~VKGSRender() override;

private:
	void prepare_rtts(rsx::framebuffer_creation_context context);

	void open_command_buffer();
	void close_and_submit_command_buffer(
		vk::fence* fence = nullptr,
		VkSemaphore wait_semaphore = VK_NULL_HANDLE,
		VkSemaphore signal_semaphore = VK_NULL_HANDLE,
		VkPipelineStageFlags pipeline_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

	void flush_command_queue(bool hard_sync = false);
	void queue_swap_request();
	void frame_context_cleanup(vk::frame_context_t *ctx, bool free_resources = false);
	void advance_queued_frames();
	void present(vk::frame_context_t *ctx);
	void reinitialize_swapchain();

	vk::image* get_present_source(vk::present_surface_info* info, const rsx::avconf* avconfig);

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
	void bind_texture_env();

public:
	void init_buffers(rsx::framebuffer_creation_context context, bool skip_reading = false);
	void set_viewport();
	void set_scissor(bool clip_viewport);
	void bind_viewport();

	void sync_hint(rsx::FIFO_hint hint, void* args) override;

	void begin_occlusion_query(rsx::reports::occlusion_query_info* query) override;
	void end_occlusion_query(rsx::reports::occlusion_query_info* query) override;
	bool check_occlusion_query_status(rsx::reports::occlusion_query_info* query) override;
	void get_occlusion_query_result(rsx::reports::occlusion_query_info* query) override;
	void discard_occlusion_query(rsx::reports::occlusion_query_info* query) override;

	// External callback in case we need to suddenly submit a commandlist unexpectedly, e.g in a violation handler
	void emergency_query_cleanup(vk::command_buffer* commands);

	// Conditional rendering
	void begin_conditional_rendering(const std::vector<rsx::reports::occlusion_query_info*>& sources) override;
	void end_conditional_rendering() override;

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

	bool on_decompiler_task() override;
};
