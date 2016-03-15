#pragma once
#include "Emu/RSX/GSRender.h"
#include "VKHelpers.h"
#include "VKTextureCache.h"
#include "VKRenderTargets.h"
#include "VKFormats.h"

#define RSX_DEBUG 1

#include "VKProgramBuffer.h"
#include "../GCM.h"

#pragma comment(lib, "VKstatic.1.lib")

namespace vk
{
// TODO: factorize between backends
class data_heap
{
	/**
	* Does alloc cross get position ?
	*/
	template<int Alignement>
	bool can_alloc(size_t size) const
	{
		size_t alloc_size = align(size, Alignement);
		size_t aligned_put_pos = align(m_put_pos, Alignement);
		if (aligned_put_pos + alloc_size < m_size)
		{
			// range before get
			if (aligned_put_pos + alloc_size < m_get_pos)
				return true;
			// range after get
			if (aligned_put_pos > m_get_pos)
				return true;
			return false;
		}
		else
		{
			// ..]....[..get..
			if (aligned_put_pos < m_get_pos)
				return false;
			// ..get..]...[...
			// Actually all resources extending beyond heap space starts at 0
			if (alloc_size > m_get_pos)
				return false;
			return true;
		}
	}

	size_t m_size;
	size_t m_put_pos; // Start of free space
public:
	data_heap() = default;
	~data_heap() = default;
	data_heap(const data_heap&) = delete;
	data_heap(data_heap&&) = delete;

	size_t m_get_pos; // End of free space

	void init(size_t heap_size)
	{
		m_size = heap_size;
		m_put_pos = 0;
		m_get_pos = heap_size - 1;
	}

	template<int Alignement>
	size_t alloc(size_t size)
	{
		if (!can_alloc<Alignement>(size)) throw EXCEPTION("Working buffer not big enough");
		size_t alloc_size = align(size, Alignement);
		size_t aligned_put_pos = align(m_put_pos, Alignement);
		if (aligned_put_pos + alloc_size < m_size)
		{
			m_put_pos = aligned_put_pos + alloc_size;
			return aligned_put_pos;
		}
		else
		{
			m_put_pos = alloc_size;
			return 0;
		}
	}

	/**
	* return current putpos - 1
	*/
	size_t get_current_put_pos_minus_one() const
	{
		return (m_put_pos - 1 > 0) ? m_put_pos - 1 : m_size - 1;
	}
};
}

class VKGSRender : public GSRender
{
private:
	VKFragmentProgram m_fragment_prog;
	VKVertexProgram m_vertex_prog;

	vk::glsl::program *m_program;
	vk::context m_thread_context;

	rsx::surface_info m_surface;

	vk::data_heap m_attrib_ring_info;
	std::unique_ptr<vk::buffer> m_attrib_buffers;
	
	vk::texture_cache m_texture_cache;
	rsx::vk_render_targets m_rtts;

	vk::gpu_formats_support m_optimal_tiling_supported_formats;
	vk::memory_type_mapping m_memory_type_mapping;

public:
	//vk::fbo draw_fbo;

private:
	VKProgramBuffer m_prog_buffer;

	vk::render_device *m_device;
	vk::swap_chain* m_swap_chain;
	//buffer

	vk::data_heap m_uniform_buffer_ring_info;
	std::unique_ptr<vk::buffer> m_uniform_buffer;
	vk::data_heap m_index_buffer_ring_info;
	std::unique_ptr<vk::buffer> m_index_buffer;

	//Vulkan internals
	u32 m_current_present_image = 0xFFFF;
	VkSemaphore m_present_semaphore = nullptr;

	u32 m_current_sync_buffer_index = 0;
	VkFence m_submit_fence = nullptr;

	vk::command_pool m_command_buffer_pool;
	vk::command_buffer m_command_buffer;
	bool recording = false;
	bool dirty_frame = true;

	//Single render pass
	VkRenderPass m_render_pass = nullptr;
	
	u32 m_draw_calls = 0;
	
	u8 m_draw_buffers_count = 0;
	vk::framebuffer m_framebuffer;

public:
	VKGSRender();
	~VKGSRender();

private:
	void clear_surface(u32 mask);
	void init_render_pass(VkFormat surface_format, VkFormat depth_format, u8 num_draw_buffers, u8 *draw_buffers);
	void destroy_render_pass();
	void execute_command_buffer(bool wait);
	void begin_command_buffer_recording();
	void end_command_buffer_recording();

	void prepare_rtts();
	/// returns primitive topology, is_indexed, index_count, offset in index buffer, index type
	std::tuple<VkPrimitiveTopology, bool, u32, VkDeviceSize, VkIndexType> upload_vertex_data();

public:
	bool load_program();
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

	bool on_access_violation(u32 address, bool is_writing) override;
};
