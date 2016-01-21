#pragma once

#include "D3D12Utils.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/RSX/GSRender.h"

#include "D3D12RenderTargetSets.h"
#include "D3D12PipelineState.h"
#include "d3dx12.h"
#include "D3D12MemoryHelpers.h"


/**
 * TODO: If you want to improve this backend, a small list of things that are unimplemented atm :
 * - Vertex texture
 * It requires adding the reading command in D3D12FragmentProgramDecompiler,
 * generates corresponding root signatures and descriptor heap binding, and ensure that code in
 * D3D12Textures.cpp doesn't contain pixel shader specific code.
 * - MSAA
 * There is no support in the gl backend for MSAA textures atm so it needs to be implemented from scratch.
 * - Depth buffer read
 * The depth buffer can be currently properly read, but for some reasons it needs a conversion from depth16/24
 * format to rgba8 format, which doesn't make sense since the PS3 doesn't make such conversion implicitly afaik.
 * - Improve caching of vertex buffers and texture
 * Vertex buffers are cached by range. This works but in some rare situation it may be wrong, for instance if 2
 * draw call use the same buffer, but the first one doesn't use all the attribute ; then the second one will use
 * the cached version and not have updated attributes. Same for texture, if format/size does change, the caching
 * system is ignoring it.
 * - Improve sync between cell and RSX
 * A lot of optimisation can be gained from using Cell and RSX latency. Cell can't read RSX generated data without
 * synchronisation. We currently only cover semaphore sync, but there are more (like implicit sync at flip) that
 * are not currently correctly signaled which leads to deadlock.
 */

/**
 * Structure used to load/unload D3D12 lib.
 */
struct D3D12DLLManagement
{
	D3D12DLLManagement();
	~D3D12DLLManagement();
};

class D3D12GSRender : public GSRender
{
private:
	/** D3D12 structures.
	 * Note: they should be declared in reverse order of destruction
	 */
	D3D12DLLManagement m_d3d12_lib;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_command_queue;
	ComPtr<struct IDXGISwapChain3> m_swap_chain;
	ComPtr<ID3D12Resource> m_backbuffer[2];
	ComPtr<ID3D12DescriptorHeap> m_backbuffer_descriptor_heap[2];
	// m_rootSignatures[N] is RS with N texture/sample
	ComPtr<ID3D12RootSignature> m_root_signatures[17][17]; // indexed by [texture count][vertex count]

	// TODO: Use a tree structure to parse more efficiently
	data_cache m_texture_cache;
	bool invalidate_address(u32 addr);

	rsx::surface_info m_surface;

	RSXVertexProgram vertex_program;
	RSXFragmentProgram fragment_program;
	PipelineStateObjectCache m_pso_cache;
	std::tuple<ComPtr<ID3D12PipelineState>, size_t, size_t> m_current_pso;

	struct
	{
		size_t m_draw_calls_duration;
		size_t m_draw_calls_count;
		size_t m_prepare_rtt_duration;
		size_t m_vertex_index_duration;
		size_t m_buffer_upload_size;
		size_t m_program_load_duration;
		size_t m_constants_duration;
		size_t m_texture_duration;
		size_t m_flip_duration;
	} m_timers;

	void reset_timer();

	struct Shader
	{
		ID3D12PipelineState *m_PSO;
		ID3D12RootSignature *m_rootSignature;
		ID3D12Resource *m_vertexBuffer;
		ID3D12DescriptorHeap *m_textureDescriptorHeap;
		ID3D12DescriptorHeap *m_samplerDescriptorHeap;
		void Init(ID3D12Device *device, ID3D12CommandQueue *gfxcommandqueue);
		void Release();
	};

	/**
	 * Stores data related to the scaling pass that turns internal
	 * render targets into presented buffers.
	 */
	Shader m_output_scaling_pass;

	/**
	 * Data used when depth buffer is converted to uchar textures.
	 */
	ID3D12PipelineState *m_convertPSO;
	ID3D12RootSignature *m_convertRootSignature;
	void initConvertShader();

	resource_storage m_per_frame_storage[2];
	resource_storage &get_current_resource_storage();
	resource_storage &get_non_current_resource_storage();

	// Textures, constants, index and vertex buffers storage
	data_heap m_buffer_data;
	data_heap m_readback_resources;
	ComPtr<ID3D12Resource> m_vertex_buffer_data;

	rsx::render_targets m_rtts;

	INT g_descriptor_stride_srv_cbv_uav;
	INT g_descriptor_stride_dsv;
	INT g_descriptor_stride_rtv;
	INT g_descriptor_stride_samplers;

	// Used to fill unused texture slot
	ID3D12Resource *m_dummy_texture;
public:
	D3D12GSRender();
	virtual ~D3D12GSRender();
private:
	void init_d2d_structures();
	void release_d2d_structures();

	void load_program();

	void set_rtt_and_ds(ID3D12GraphicsCommandList *command_list);

	/**
	 * Create vertex and index buffers (if needed) and set them to cmdlist.
	 * Non native primitive type are emulated by index buffers expansion.
	 * Returns whether the draw call is indexed or not and the vertex count to draw.
	*/
	std::tuple<bool, size_t, std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> > upload_and_set_vertex_index_data(ID3D12GraphicsCommandList *command_list);

	/**
	 * Upload all enabled vertex attributes for vertex in ranges described by vertex_ranges.
	 * A range in vertex_range is a pair whose first element is the index of the beginning of the
	 * range, and whose second element is the number of vertex in this range.
	 */
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> upload_vertex_attributes(const std::vector<std::pair<u32, u32> > &vertex_ranges,
		gsl::not_null<ID3D12GraphicsCommandList*> command_list);

	std::tuple<D3D12_VERTEX_BUFFER_VIEW, size_t> upload_inlined_vertex_array();

	std::tuple<D3D12_INDEX_BUFFER_VIEW, size_t> generate_index_buffer_for_emulated_primitives_array(const std::vector<std::pair<u32, u32> > &vertex_ranges);

	void upload_and_bind_scale_offset_matrix(size_t descriptor_index);
	void upload_and_bind_vertex_shader_constants(size_t descriptor_index);
	void upload_and_bind_fragment_shader_constants(size_t descriptorIndex);
	/**
	 * Fetch all textures recorded in the state in the render target cache and in the texture cache.
	 * If a texture is not cached, populate cmdlist with uploads command.
	 * Create necessary resource view/sampler descriptors in the per frame storage struct.
	 * If the count of enabled texture is below texture_count, fills with dummy texture and sampler.
	 */
	void upload_and_bind_textures(ID3D12GraphicsCommandList *command_list, size_t descriptor_index, size_t texture_count);

	/**
	 * Creates render target if necessary.
	 * Populate cmdlist with render target state change (from RTT to generic read for previous rtt,
	 * from generic to rtt for rtt in cache).
	 */
	void prepare_render_targets(ID3D12GraphicsCommandList *command_list);

	/**
	 * Render D2D overlay if enabled on top of the backbuffer.
	 */
	void render_overlay();

	void clear_surface(u32 arg);

	/**
	 * Copy currently bound current target to the dma location affecting them.
	 * NOTE: We should also copy previously bound rtts.
	 */
	void copy_render_target_to_dma_location();

protected:
	virtual void on_exit() override;
	virtual bool do_method(u32 cmd, u32 arg) override;
	virtual void end() override;
	virtual void flip(int buffer) override;

	virtual std::array<std::vector<gsl::byte>, 4> copy_render_targets_to_memory() override;
	virtual std::array<std::vector<gsl::byte>, 2> copy_depth_stencil_buffer_to_memory() override;
	virtual std::pair<std::string, std::string> get_programs() const override;
};
