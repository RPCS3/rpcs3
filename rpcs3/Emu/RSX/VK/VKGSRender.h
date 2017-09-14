#pragma once
#include "Emu/RSX/GSRender.h"
#include "VKHelpers.h"
#include "VKTextureCache.h"
#include "VKRenderTargets.h"
#include "VKFormats.h"
#include "VKTextOut.h"
#include "restore_new.h"
#include <Utilities/optional.hpp>
#include "define_new_memleakdetect.h"
#include "VKProgramBuffer.h"
#include "../GCM.h"
#include "../rsx_utils.h"
#include <thread>
#include <atomic>

#pragma comment(lib, "VKstatic.1.lib")

namespace vk
{
	using vertex_cache = rsx::vertex_cache::default_vertex_cache<rsx::vertex_cache::uploaded_range<VkFormat>, VkFormat>;
	using weak_vertex_cache = rsx::vertex_cache::weak_vertex_cache<VkFormat>;
	using null_vertex_cache = vertex_cache;

	using shader_cache = rsx::shaders_cache<vk::pipeline_props, VKProgramBuffer>;
}

//Heap allocation sizes in MB
#define VK_ATTRIB_RING_BUFFER_SIZE_M 256
#define VK_UBO_RING_BUFFER_SIZE_M 64
#define VK_INDEX_RING_BUFFER_SIZE_M 64
#define VK_TEXTURE_UPLOAD_RING_BUFFER_SIZE_M 128

#define VK_MAX_ASYNC_CB_COUNT 64
#define VK_MAX_ASYNC_FRAMES 2

extern u64 get_system_time();

struct command_buffer_chunk: public vk::command_buffer
{
	VkFence submit_fence = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;

	std::atomic_bool pending = { false };
	std::atomic<u64> last_sync = { 0 };

	command_buffer_chunk()
	{}

	void init_fence(VkDevice dev)
	{
		m_device = dev;

		VkFenceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		vkCreateFence(m_device, &info, nullptr, &submit_fence);
	}

	void destroy()
	{
		vk::command_buffer::destroy();

		if (submit_fence != VK_NULL_HANDLE)
			vkDestroyFence(m_device, submit_fence, nullptr);
	}

	void tag()
	{
		last_sync = get_system_time();
	}

	void reset()
	{
		if (pending)
			poke();

		if (pending)
			wait();

		vkResetCommandBuffer(commands, 0);
	}

	void poke()
	{
		if (vkGetFenceStatus(m_device, submit_fence) == VK_SUCCESS)
		{
			vkResetFences(m_device, 1, &submit_fence);
			pending = false;
		}
	}

	void wait()
	{
		if (!pending)
			return;

		switch(vkGetFenceStatus(m_device, submit_fence))
		{
		case VK_SUCCESS:
			break;
		case VK_NOT_READY:
			vkWaitForFences(m_device, 1, &submit_fence, VK_TRUE, UINT64_MAX);
			break;
		}

		vkResetFences(m_device, 1, &submit_fence);
		pending = false;
	}
};

class VKGSRender : public GSRender
{
private:
	VKFragmentProgram m_fragment_prog;
	VKVertexProgram m_vertex_prog;

	vk::glsl::program *m_program;
	vk::context m_thread_context;

	vk::vk_data_heap m_attrib_ring_info;
	
	vk::texture_cache m_texture_cache;
	rsx::vk_render_targets m_rtts;

	vk::gpu_formats_support m_optimal_tiling_supported_formats;
	vk::memory_type_mapping m_memory_type_mapping;

	std::unique_ptr<vk::buffer> null_buffer;
	std::unique_ptr<vk::buffer_view> null_buffer_view;

	std::unique_ptr<vk::text_writer> m_text_writer;

public:
	//vk::fbo draw_fbo;
	std::unique_ptr<vk::vertex_cache> m_vertex_cache;
	std::unique_ptr<vk::shader_cache> m_shaders_cache;

private:
	std::unique_ptr<VKProgramBuffer> m_prog_buffer;

	vk::render_device *m_device;
	vk::swap_chain* m_swap_chain;

	//buffer
	vk::vk_data_heap m_uniform_buffer_ring_info;
	vk::vk_data_heap m_index_buffer_ring_info;
	vk::vk_data_heap m_texture_upload_buffer_ring_info;
	std::unique_ptr<vk::buffer_view> m_null_buffer_view;

	//Vulkan internals
	vk::command_pool m_command_buffer_pool;

	std::mutex m_secondary_cb_guard;
	vk::command_pool m_secondary_command_buffer_pool;
	vk::command_buffer m_secondary_command_buffer;

	u32 m_current_cb_index = 0;
	std::array<command_buffer_chunk, VK_MAX_ASYNC_CB_COUNT> m_primary_cb_list;
	command_buffer_chunk* m_current_command_buffer = nullptr;

	std::array<VkRenderPass, 120> m_render_passes;

	VkDescriptorSetLayout descriptor_layouts;
	VkPipelineLayout pipeline_layout;

	std::unique_ptr<vk::framebuffer_holder> m_draw_fbo;

	struct frame_context_t
	{
		VkSemaphore present_semaphore = VK_NULL_HANDLE;
		VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
		vk::descriptor_pool descriptor_pool;
		u32 used_descriptors = 0;

		std::vector<std::unique_ptr<vk::buffer_view>> buffer_views_to_clean;
		std::vector<std::unique_ptr<vk::sampler>> samplers_to_clean;

		u32 present_image = UINT32_MAX;
		command_buffer_chunk* swap_command_buffer = nullptr;

		//Copy shareable information
		void grab_resources(frame_context_t &other)
		{
			present_semaphore = other.present_semaphore;
			descriptor_set = other.descriptor_set;
			descriptor_pool = other.descriptor_pool;
			used_descriptors = other.used_descriptors;
		}

		//Exchange storage (non-copyable)
		void swap_storage(frame_context_t &other)
		{
			std::swap(buffer_views_to_clean, other.buffer_views_to_clean);
			std::swap(samplers_to_clean, other.samplers_to_clean);
		}
	};

	std::array<frame_context_t, VK_MAX_ASYNC_FRAMES> frame_context_storage;
	//Temp frame context to use if the real frame queue is overburdened. Only used for storage
	frame_context_t m_aux_frame_context;

	//framebuffers are shared between frame contexts
	std::list<std::unique_ptr<vk::framebuffer_holder>> m_framebuffers_to_clean;

	u32 m_current_queue_index = 0;
	frame_context_t* m_current_frame = nullptr;

	u32 m_client_width = 0;
	u32 m_client_height = 0;

	// Draw call stats
	u32 m_draw_calls = 0;
	u32 m_instanced_draws = 0;

	// Vertex buffer usage stats
	u32 m_uploads_small = 0;
	u32 m_uploads_1k = 0;
	u32 m_uploads_2k = 0;
	u32 m_uploads_4k = 0;
	u32 m_uploads_8k = 0;
	u32 m_uploads_16k = 0;

	// Timers
	s64 m_setup_time = 0;
	s64 m_vertex_upload_time = 0;
	s64 m_textures_upload_time = 0;
	s64 m_draw_time = 0;
	s64 m_flip_time = 0;

	u8 m_draw_buffers_count = 0;

	bool framebuffer_status_valid = false;

	rsx::gcm_framebuffer_info m_surface_info[rsx::limits::color_buffers_count];
	rsx::gcm_framebuffer_info m_depth_surface_info;

	bool m_flush_draw_buffers = false;
	std::atomic<int> m_last_flushable_cb = {-1 };
	
	std::mutex m_flush_queue_mutex;
	std::atomic<bool> m_flush_commands = { false };
	std::atomic<int> m_queued_threads = { 0 };

	std::thread::id rsx_thread;
	std::atomic<u64> m_last_sync_event = { 0 };

	bool render_pass_open = false;

	//Vertex layout
	rsx::vertex_input_layout m_vertex_layout;
	
#if !defined(_WIN32) && defined(HAVE_VULKAN)
	Display *m_display_handle = nullptr;
#endif

public:
	VKGSRender();
	~VKGSRender();

private:
	void clear_surface(u32 mask);
	void close_and_submit_command_buffer(const std::vector<VkSemaphore> &semaphores, VkFence fence, VkPipelineStageFlags pipeline_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	void open_command_buffer();
	void sync_at_semaphore_release();
	void prepare_rtts();
	void copy_render_targets_to_dma_location();

	void flush_command_queue(bool hard_sync = false);
	void queue_swap_request();
	void process_swap_request(frame_context_t *ctx, bool free_resources = false);
	void advance_queued_frames();
	void present(frame_context_t *ctx);

	void begin_render_pass();
	void close_render_pass();

	/// returns primitive topology, index_count, allocated_verts, vertex_base_index, (offset in index buffer, index type)
	std::tuple<VkPrimitiveTopology, u32, u32, u32, std::optional<std::tuple<VkDeviceSize, VkIndexType> > > upload_vertex_data();
public:
	bool check_program_status();
	void load_program(u32 vertex_count, u32 vertex_base);
	void init_buffers(bool skip_reading = false);
	void read_buffers();
	void write_buffers();
	void set_viewport();

protected:
	void begin() override;
	void end() override;

	void on_init_thread() override;
	void on_exit() override;
	bool do_method(u32 id, u32 arg) override;
	void flip(int buffer) override;

	void do_local_task() override;
	bool scaled_image_from_memory(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate) override;

	bool on_access_violation(u32 address, bool is_writing) override;
	void on_notify_memory_unmapped(u32 address_base, u32 size) override;
};
