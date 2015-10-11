#pragma once
#if defined(DX12_SUPPORT)

#include "D3D12.h"
#include "rpcs3/Ini.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Utilities/File.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/RSX/GSRender.h"

#include "D3D12RenderTargetSets.h"
#include "D3D12PipelineState.h"
#include "D3D12Buffer.h"
#include "d3dx12.h"

// Some constants are the same between RSX and GL
#include <GL\GL.h>


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

template<typename T>
struct InitHeap
{
	static T* Init(ID3D12Device *device, size_t heapSize, D3D12_HEAP_TYPE type, D3D12_HEAP_FLAGS flags);
};

template<>
struct InitHeap<ID3D12Heap>
{
	static ID3D12Heap* Init(ID3D12Device *device, size_t heapSize, D3D12_HEAP_TYPE type, D3D12_HEAP_FLAGS flags)
	{
		ID3D12Heap *result;
		D3D12_HEAP_DESC heapDesc = {};
		heapDesc.SizeInBytes = heapSize;
		heapDesc.Properties.Type = type;
		heapDesc.Flags = flags;
		ThrowIfFailed(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&result)));
		return result;
	}
};

template<>
struct InitHeap<ID3D12Resource>
{
	static ID3D12Resource* Init(ID3D12Device *device, size_t heapSize, D3D12_HEAP_TYPE type, D3D12_HEAP_FLAGS flags)
	{
		ID3D12Resource *result;
		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = type;
		ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
			flags,
			&CD3DX12_RESOURCE_DESC::Buffer(heapSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&result))
			);

		return result;
	}
};


/**
 * Wrapper around a ID3D12Resource or a ID3D12Heap.
 * Acts as a ring buffer : hold a get and put pointers,
 * put pointer is used as storage space offset
 * and get is used as beginning of in use data space.
 * This wrapper checks that put pointer doesn't cross get one.
 */
template<typename T, size_t Alignment>
struct DataHeap
{
	T *m_heap;
	size_t m_size;
	size_t m_putPos; // Start of free space
	size_t m_getPos; // End of free space

	void Init(ID3D12Device *device, size_t heapSize, D3D12_HEAP_TYPE type, D3D12_HEAP_FLAGS flags)
	{
		m_size = heapSize;
		m_heap = InitHeap<T>::Init(device, heapSize, type, flags);
		m_putPos = 0;
		m_getPos = heapSize - 1;
	}

	/**
	* Does alloc cross get position ?
	*/
	bool canAlloc(size_t size) const
	{
		size_t allocSize = align(size, Alignment);
		size_t currentGetPos = m_getPos;
		if (m_putPos + allocSize < m_size)
		{
			// range before get
			if (m_putPos + allocSize < m_getPos)
				return true;
			// range after get
			if (m_putPos > m_getPos)
				return true;
			return false;
		}
		else
		{
			// ..]....[..get..
			if (m_putPos < m_getPos)
				return false;
			// ..get..]...[...
			// Actually all resources extending beyond heap space starts at 0
			if (allocSize > m_getPos)
				return false;
			return true;
		}
	}

	size_t alloc(size_t size)
	{
		assert(canAlloc(size));
		size_t allocSize = align(size, Alignment);
		if (m_putPos + allocSize < m_size)
		{
			size_t oldPutPos = m_putPos;
			m_putPos += allocSize;
			return oldPutPos;
		}
		else
		{
			m_putPos = allocSize;
			return 0;
		}
	}

	void Release()
	{
		m_heap->Release();
	}

	/**
	 * return current putpos - 1
	 */
	size_t getCurrentPutPosMinusOne() const
	{
		return (m_putPos - 1 > 0) ? m_putPos - 1 : m_size - 1;
	}
};

struct TextureEntry
{
	int m_format;
	size_t m_width;
	size_t m_height;
	size_t m_mipmap;
	bool m_isDirty;

	TextureEntry() : m_format(0), m_width(0), m_height(0), m_isDirty(true)
	{}

	TextureEntry(int f, size_t w, size_t h, size_t m) : m_format(f), m_width(w), m_height(h), m_isDirty(false)
	{}

	bool operator==(const TextureEntry &other)
	{
		return (m_format == other.m_format && m_width == other.m_width && m_height == other.m_height);
	}
};

/**
 * Manages cache of data (texture/vertex/index)
 */
struct DataCache
{
private:
	/**
	* Mutex protecting m_dataCache access
	* Memory protection fault catch can be generated by any thread and
	* modifies it.
	*/
	std::mutex mut;

	std::unordered_map<u64, std::pair<TextureEntry, ComPtr<ID3D12Resource>> > m_dataCache; // Storage
	std::list <std::tuple<u64, u32, u32> > m_protectedRange; // address, start of protected range, size of protected range
public:
	void storeAndProtectData(u64 key, u32 start, size_t size, int format, size_t w, size_t h, size_t m, ComPtr<ID3D12Resource> data)
	{
		std::lock_guard<std::mutex> lock(mut);
		m_dataCache[key] = std::make_pair(TextureEntry(format, w, h, m), data);
		protectData(key, start, size);
	}

	/**
	 * Make memory from start to start + size write protected.
	 * Associate key to this range so that when a write is detected, data at key is marked dirty.
	 */
	void protectData(u64 key, u32 start, size_t size)
	{
		/// align start to 4096 byte
		u32 protected_range_start = align(start, 4096);
		u32 protected_range_size = (u32)align(size, 4096);
		m_protectedRange.push_back(std::make_tuple(key, protected_range_start, protected_range_size));
		vm::page_protect(protected_range_start, protected_range_size, 0, 0, vm::page_writable);
	}

	/// remove all data containing addr from cache, unprotect them. Returns false if no data is modified.
	bool invalidateAddress(u32 addr)
	{
		bool handled = false;
		auto It = m_protectedRange.begin(), E = m_protectedRange.end();
		for (; It != E;)
		{
			auto currentIt = It;
			++It;
			auto protectedTexture = *currentIt;
			u32 protectedRangeStart = std::get<1>(protectedTexture), protectedRangeSize = std::get<2>(protectedTexture);
			if (addr >= protectedRangeStart && addr <= protectedRangeSize + protectedRangeStart)
			{
				std::lock_guard<std::mutex> lock(mut);
				u64 texadrr = std::get<0>(protectedTexture);
				m_dataCache[texadrr].first.m_isDirty = true;

				vm::page_protect(protectedRangeStart, protectedRangeSize, 0, vm::page_writable, 0);
				m_protectedRange.erase(currentIt);
				handled = true;
			}
		}
		return handled;
	}

	std::pair<TextureEntry, ComPtr<ID3D12Resource> > *findDataIfAvailable(u64 key)
	{
		std::lock_guard<std::mutex> lock(mut);
		auto It = m_dataCache.find(key);
		if (It == m_dataCache.end())
			return nullptr;
		return &It->second;
	}

	void unprotedAll()
	{
		std::lock_guard<std::mutex> lock(mut);
		for (auto &protectedTexture : m_protectedRange)
		{
			u32 protectedRangeStart = std::get<1>(protectedTexture), protectedRangeSize = std::get<2>(protectedTexture);
			vm::page_protect(protectedRangeStart, protectedRangeSize, 0, vm::page_writable, 0);
		}
	}

	/**
	 * Remove data stored at key, and returns a ComPtr owning it.
	 * The caller is responsible for releasing the ComPtr.
	 */
	ComPtr<ID3D12Resource> removeFromCache(u64 key)
	{
		auto result = m_dataCache[key].second;
		m_dataCache.erase(key);
		return result;
	}
};

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
	D3D12DLLManagement m_D3D12Lib;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueueGraphic;
	ComPtr<struct IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Resource> m_backBuffer[2];
	ComPtr<ID3D12DescriptorHeap> m_backbufferAsRendertarget[2];
	// m_rootSignatures[N] is RS with N texture/sample
	ComPtr<ID3D12RootSignature> m_rootSignatures[17];

	// TODO: Use a tree structure to parse more efficiently
	DataCache m_textureCache;
	bool invalidateAddress(u32 addr);

	// Copy of RTT to be used as texture
	std::unordered_map<u32, ID3D12Resource* > m_texturesRTTs;

	RSXFragmentProgram fragment_program;
	PipelineStateObjectCache m_cachePSO;
	std::pair<ID3D12PipelineState *, size_t> *m_PSO;

	size_t m_vertexBufferSize[32];

	struct
	{
		size_t m_drawCallDuration;
		size_t m_drawCallCount;
		size_t m_rttDuration;
		size_t m_vertexIndexDuration;
		size_t m_bufferUploadSize;
		size_t m_programLoadDuration;
		size_t m_constantsDuration;
		size_t m_textureDuration;
		size_t m_flipDuration;
	} m_timers;

	void ResetTimer();

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
	Shader m_outputScalingPass;

	/**
	 * Data used when depth buffer is converted to uchar textures.
	 */
	ID3D12PipelineState *m_convertPSO;
	ID3D12RootSignature *m_convertRootSignature;
	void initConvertShader();


	/**
	 * Stores data that are "ping ponged" between frame.
	 * For instance command allocator : maintains 2 command allocators and
	 * swap between them when frame is flipped.
	 */
	struct ResourceStorage
	{
		bool m_inUse; // False until command list has been populated at least once
		ComPtr<ID3D12Fence> m_frameFinishedFence;
		UINT64 m_fenceValue;
		HANDLE m_frameFinishedHandle;

		// Pointer to device, not owned by ResourceStorage
		ID3D12Device *m_device;
		ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		ComPtr<ID3D12GraphicsCommandList> m_commandList;

		// Constants storage
		ComPtr<ID3D12DescriptorHeap> m_constantsBufferDescriptorsHeap;
		size_t m_constantsBufferIndex;
		ComPtr<ID3D12DescriptorHeap> m_scaleOffsetDescriptorHeap;
		size_t m_currentScaleOffsetBufferIndex;

		// Texture storage
		ComPtr<ID3D12DescriptorHeap> m_textureDescriptorsHeap;
		size_t m_currentTextureIndex;
		ComPtr<ID3D12DescriptorHeap> m_samplerDescriptorHeap[2];
		size_t m_samplerDescriptorHeapIndex;
		size_t m_currentSamplerIndex;

		ComPtr<ID3D12Resource> m_RAMFramebuffer;

		// List of resources that can be freed after frame is flipped
		std::vector<ComPtr<ID3D12Resource> > m_singleFrameLifetimeResources;


		/// Texture that were invalidated
		std::list<ComPtr<ID3D12Resource> > m_dirtyTextures;

		size_t m_getPosConstantsHeap;
		size_t m_getPosVertexIndexHeap;
		size_t m_getPosTextureUploadHeap;
		size_t m_getPosReadbackHeap;
		size_t m_getPosUAVHeap;

		void Reset();
		void Init(ID3D12Device *device);
		void setNewCommandList();
		void WaitAndClean();
		void Release();
	};

	ResourceStorage m_perFrameStorage[2];
	ResourceStorage &getCurrentResourceStorage();
	ResourceStorage &getNonCurrentResourceStorage();

	// Constants storage
	DataHeap<ID3D12Resource, 256> m_constantsData;
	// Vertex storage
	DataHeap<ID3D12Resource, 65536> m_vertexIndexData;
	// Texture storage
	DataHeap<ID3D12Resource, 65536> m_textureUploadData;
	DataHeap<ID3D12Heap, 65536> m_UAVHeap;
	DataHeap<ID3D12Heap, 65536> m_readbackResources;

	struct
	{
		bool m_indexed; /*<! is draw call using an index buffer */
		size_t m_count; /*<! draw call vertex count */
		size_t m_baseVertex; /*<! Starting vertex for draw call */
	} m_renderingInfo;

	RenderTargets m_rtts;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_IASet;

	INT g_descriptorStrideSRVCBVUAV;
	INT g_descriptorStrideDSV;
	INT g_descriptorStrideRTV;
	INT g_descriptorStrideSamplers;

	// Used to fill unused texture slot
	ID3D12Resource *m_dummyTexture;

	size_t m_lastWidth, m_lastHeight, m_lastDepth;

	// Store previous fbo addresses to detect RTT config changes.
	u32 m_previous_address_a;
	u32 m_previous_address_b;
	u32 m_previous_address_c;
	u32 m_previous_address_d;
	u32 m_previous_address_z;
public:
	u32 m_draw_frames;
	u32 m_skip_frames;

	D3D12GSRender();
	virtual ~D3D12GSRender();

	virtual void semaphorePGRAPHTextureReadRelease(u32 offset, u32 value) override;
	virtual void semaphorePGRAPHBackendRelease(u32 offset, u32 value) override;
	virtual void semaphorePFIFOAcquire(u32 offset, u32 value) override;
	virtual void notifyProgramChange() override;
	virtual void notifyBlendStateChange() override;
	virtual void notifyDepthStencilStateChange() override;
	virtual void notifyRasterizerStateChange() override;

private:
	void InitD2DStructures();
	void ReleaseD2DStructures();
	ID3D12Resource *writeColorBuffer(ID3D12Resource *RTT, ID3D12GraphicsCommandList *cmdlist);

	bool LoadProgram();

	/**
	 * Create as little vertex buffer as possible to hold all vertex info (in upload heap),
	 * create corresponding IA layout that can be used for load program and
	 * returns a vector of vertex buffer view that can be passed to IASetVertexBufferView().
	 */
	std::vector<D3D12_VERTEX_BUFFER_VIEW> UploadVertexBuffers(bool indexed_draw = false);

	/**
	 * Create index buffer for indexed rendering and non native primitive format if nedded, and
	 * update m_renderingInfo member accordingly. If m_renderingInfo::m_indexed is true,
	 * returns an index buffer view that can be passed to a command list.
	 */
	D3D12_INDEX_BUFFER_VIEW uploadIndexBuffers(bool indexed_draw = false);


	void setScaleOffset();
	void FillVertexShaderConstantsBuffer();
	void FillPixelShaderConstantsBuffer();
	/**
	 * Fetch all textures recorded in the state in the render target cache and in the texture cache.
	 * If a texture is not cached, populate cmdlist with uploads command.
	 * Create necessary resource view/sampler descriptors in the per frame storage struct.
	 * returns the number of texture uploaded.
	 */
	size_t UploadTextures(ID3D12GraphicsCommandList *cmdlist);

	/**
	 * Creates render target if necessary.
	 * Populate cmdlist with render target state change (from RTT to generic read for previous rtt,
	 * from generic to rtt for rtt in cache).
	 */
	void PrepareRenderTargets(ID3D12GraphicsCommandList *cmdlist);

	/**
	 * Render D2D overlay if enabled on top of the backbuffer.
	 */
	void renderOverlay();

	void clear_surface(u32 arg);

protected:
	virtual void onexit_thread() override;
	virtual void OnReset() override;
	virtual bool domethod(u32 cmd, u32 arg) override;
	virtual void end() override;
	virtual void flip(int buffer) override;
};

#endif