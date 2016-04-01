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

class VKGSRender : public GSRender
{
private:
	VKFragmentProgram m_fragment_prog;
	VKVertexProgram m_vertex_prog;

	vk::glsl::program *m_program;
	vk::context m_thread_context;

	rsx::surface_info m_surface;

	vk::vk_data_heap m_attrib_ring_info;
	
	vk::texture_cache m_texture_cache;
	rsx::vk_render_targets m_rtts;

	vk::gpu_formats_support m_optimal_tiling_supported_formats;
	vk::memory_type_mapping m_memory_type_mapping;

	std::unique_ptr<vk::buffer> null_buffer;
	std::unique_ptr<vk::buffer_view> null_buffer_view;

public:
	//vk::fbo draw_fbo;

private:
	VKProgramBuffer m_prog_buffer;

	vk::render_device *m_device;
	vk::swap_chain* m_swap_chain;
	//buffer

	vk::vk_data_heap m_uniform_buffer_ring_info;
	vk::vk_data_heap m_index_buffer_ring_info;
	vk::vk_data_heap m_texture_upload_buffer_ring_info;

	//Vulkan internals
	u32 m_current_present_image = 0xFFFF;
	VkSemaphore m_present_semaphore = nullptr;

	u32 m_current_sync_buffer_index = 0;
	VkFence m_submit_fence = nullptr;

	vk::command_pool m_command_buffer_pool;
	vk::command_buffer m_command_buffer;


	std::array<VkRenderPass, 120> m_render_passes;
	VkDescriptorSetLayout descriptor_layouts;
	VkDescriptorSet descriptor_sets;
	VkPipelineLayout pipeline_layout;
	vk::descriptor_pool descriptor_pool;

	std::vector<std::unique_ptr<vk::buffer_view> > m_buffer_view_to_clean;
	std::vector<std::unique_ptr<vk::framebuffer> > m_framebuffer_to_clean;
	std::vector<std::unique_ptr<vk::sampler> > m_sampler_to_clean;

	u32 m_draw_calls = 0;
	u8 m_draw_buffers_count = 0;

public:
	VKGSRender();
	~VKGSRender();

private:
	void clear_surface(u32 mask);
	void close_and_submit_command_buffer(const std::vector<VkSemaphore> &semaphores, VkFence fence);
	void open_command_buffer();
	void sync_at_semaphore_release();
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
