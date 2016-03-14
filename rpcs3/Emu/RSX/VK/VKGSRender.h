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

	vk::buffer m_attrib_buffers[rsx::limits::vertex_count];
	
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

	vk::buffer m_scale_offset_buffer;
	vk::buffer m_vertex_constants_buffer;
	vk::buffer m_fragment_constants_buffer;

	vk::buffer m_index_buffer;

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
	
	std::tuple<VkPrimitiveTopology, bool, u32, VkIndexType>
	upload_vertex_data();

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
