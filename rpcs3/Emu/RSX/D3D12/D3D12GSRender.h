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
 * - Fix vertex buffer in The Guided Paradox
 * The vertex info in the guided paradox are wrong, leading to missing character parts ingame (like leg or torso).
 * It's because some vertex position are incorrect.
 * - Improve sync between cell and RSX
 * A lot of optimisation can be gained from using Cell and RSX latency. Cell can't read RSX generated data without
 * synchronisation. We currently only cover semaphore sync, but there are more (like implicit sync at flip) that
 * are not currently correctly signaled which leads to deadlock.
 */

class GSFrameBase2
{
public:
	GSFrameBase2() {}
	GSFrameBase2(const GSFrameBase2&) = delete;
	virtual void Close() = 0;

	virtual bool IsShown() = 0;
	virtual void Hide() = 0;
	virtual void Show() = 0;

	virtual void* GetNewContext() = 0;
	virtual void SetCurrent(void* ctx) = 0;
	virtual void DeleteContext(void* ctx) = 0;
	virtual void Flip(void* ctx) = 0;
	virtual HWND getHandle() const = 0;
	virtual void SetAdaptaterName(const wchar_t *) = 0;
};

typedef GSFrameBase2*(*GetGSFrameCb2)();

void SetGetD3DGSFrameCallback(GetGSFrameCb2 value);

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
		check(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&result)));
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
		check(device->CreateCommittedResource(&heapProperties,
			flags,
			&getBufferResourceDesc(heapSize),
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
	std::atomic<size_t> m_getPos; // End of free space
	std::vector<std::tuple<size_t, size_t, ID3D12Resource *> > m_resourceStoredSinceLastSync;

	void Init(ID3D12Device *device, size_t heapSize, D3D12_HEAP_TYPE type, D3D12_HEAP_FLAGS flags)
	{
		m_size = heapSize;
		m_heap = InitHeap<T>::Init(device, heapSize, type, flags);
		m_putPos = 0;
		m_getPos = m_size - 1;
	}

	/**
	* Does alloc cross get position ?
	*/
	bool canAlloc(size_t size) const
	{
		size_t allocSize = align(size, Alignment);
		size_t currentGetPos = m_getPos.load();
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
		for (auto tmp : m_resourceStoredSinceLastSync)
		{
			SAFE_RELEASE(std::get<2>(tmp));
		}
	}

	/**
	 * Get a function that cleans heaps.
	 * It's caller responsability to ensure data are not used when executed.
	 */
	std::function<void()> getCleaningFunction()
	{
		std::atomic<size_t>& getPointer = m_getPos;
		auto duplicatem_resourceStoredSinceLastSync = m_resourceStoredSinceLastSync;
		m_resourceStoredSinceLastSync.clear();
		return [=, &getPointer]() {
			for (auto tmp : duplicatem_resourceStoredSinceLastSync)
			{
				SAFE_RELEASE(std::get<2>(tmp));
				getPointer.exchange(std::get<0>(tmp));
			}
		};
	}
};


/**
 * Wrapper for a worker thread that executes lambda functions
 * in the order they were submitted during its lifetime.
 * Used mostly to release data that are not needed anymore.
 */
struct GarbageCollectionThread
{
	std::mutex m_mutex;
	std::condition_variable cv;
	std::queue<std::function<void()> > m_queue;
	std::thread m_worker;

	GarbageCollectionThread();
	~GarbageCollectionThread();
	void pushWork(std::function<void()>&& f);
	void waitForCompletion();
};

class D3D12GSRender : public GSRender
{
private:
	/**
	 * Mutex protecting m_texturesCache and m_Textoclean access
	 * Memory protection fault catch can be generated by any thread and
	 * modifies these two members.
	 */
	std::mutex mut;
	std::list <std::tuple<u32, u32, u32> > m_protectedTextures; // Texaddress, start of protected range, size of protected range
	std::vector<ID3D12Resource *> m_texToClean;

	GarbageCollectionThread m_GC;
	// Copy of RTT to be used as texture
	std::unordered_map<u32, ID3D12Resource* > m_texturesRTTs;

	std::unordered_map<u32, ID3D12Resource*> m_texturesCache;
	//  std::vector<PostDrawObj> m_post_draw_objs;

	// TODO: Use a tree structure to parse more efficiently
	// Key is begin << 32 | end
	std::unordered_map<u64, ID3D12Resource *> m_vertexCache;

	PipelineStateObjectCache m_cachePSO;
	std::pair<ID3D12PipelineState *, size_t> *m_PSO;
	// m_rootSignatures[N] is RS with N texture/sample
	ID3D12RootSignature *m_rootSignatures[17];

	struct
	{
		size_t m_vertexUploadDuration;
		size_t m_textureUploadDuration;
	} m_timers;

	void ResetTimer();

	struct Shader
	{
		ID3D12PipelineState *m_PSO;
		ID3D12RootSignature *m_rootSignature;
		ID3D12Resource *m_vertexBuffer;
		ID3D12DescriptorHeap *m_textureDescriptorHeap;
		ID3D12DescriptorHeap *m_samplerDescriptorHeap;
		void Init(ID3D12Device *device);
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


	/**
	 * Stores data that are "ping ponged" between frame.
	 * For instance command allocator : maintains 2 command allocators and
	 * swap between them when frame is flipped.
	 */
	struct ResourceStorage
	{
		ID3D12Fence* m_frameFinishedFence;
		HANDLE m_frameFinishedHandle;
		ID3D12CommandAllocator *m_commandAllocator;
		ID3D12CommandAllocator *m_downloadCommandAllocator;
		std::list<ID3D12GraphicsCommandList *> m_inflightCommandList;

		// Constants storage
		ID3D12DescriptorHeap *m_constantsBufferDescriptorsHeap;
		size_t m_constantsBufferIndex;
		ID3D12DescriptorHeap *m_scaleOffsetDescriptorHeap;
		size_t m_currentScaleOffsetBufferIndex;

		// Texture storage
		ID3D12CommandAllocator *m_textureUploadCommandAllocator;
		ID3D12DescriptorHeap *m_textureDescriptorsHeap;
		ID3D12DescriptorHeap *m_samplerDescriptorHeap;
		size_t m_currentTextureIndex;

		void Reset();
		void Init(ID3D12Device *device);
		void Release();
	};

	ResourceStorage m_perFrameStorage[2];
	ResourceStorage &getCurrentResourceStorage();
	ResourceStorage &getNonCurrentResourceStorage();

	// Constants storage
	DataHeap<ID3D12Resource, 256> m_constantsData;
	// Vertex storage
	DataHeap<ID3D12Heap, 65536> m_vertexIndexData;
	// Texture storage
	DataHeap<ID3D12Heap, 65536> m_textureUploadData;
	DataHeap<ID3D12Heap, 65536> m_UAVHeap;
	DataHeap<ID3D12Heap, 65536> m_readbackResources;

	bool m_forcedIndexBuffer;
	size_t indexCount;

	RenderTargets m_rtts;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_IASet;
	ID3D12Device* m_device;
	size_t g_descriptorStrideSRVCBVUAV;
	size_t g_descriptorStrideDSV;
	size_t g_descriptorStrideRTV;
	size_t g_descriptorStrideSamplers;
	ID3D12CommandQueue *m_commandQueueCopy;
	ID3D12CommandQueue *m_commandQueueGraphic;

	// Used to fill unused texture slot
	ID3D12Resource *m_dummyTexture;

	struct IDXGISwapChain3 *m_swapChain;
	//BackBuffers
	ID3D12Resource* m_backBuffer[2];
	ID3D12DescriptorHeap *m_backbufferAsRendertarget[2];

	size_t m_lastWidth, m_lastHeight, m_lastDepth;
public:
	GSFrameBase2 *m_frame;
	u32 m_draw_frames;
	u32 m_skip_frames;

	std::unordered_map<size_t, RSXTransformConstant> m_vertexConstants;

	D3D12GSRender();
	virtual ~D3D12GSRender();

	virtual void semaphorePGRAPHBackendRelease(u32 offset, u32 value) override;
	virtual void semaphorePFIFOAcquire(u32 offset, u32 value) override;

private:
	ID3D12Resource *writeColorBuffer(ID3D12Resource *RTT, ID3D12GraphicsCommandList *cmdlist);
	virtual void Close() override;

	bool LoadProgram();
	std::pair<std::vector<D3D12_VERTEX_BUFFER_VIEW>, D3D12_INDEX_BUFFER_VIEW> UploadVertexBuffers(bool indexed_draw = false);
	void setScaleOffset();
	void FillVertexShaderConstantsBuffer();
	void FillPixelShaderConstantsBuffer();
	/**
	 * Upload textures to Data heap if necessary and create necessary descriptor in the per frame storage struct.
	 * returns the number of texture uploaded
	 */
	size_t UploadTextures();
	size_t GetMaxAniso(size_t aniso);
	D3D12_TEXTURE_ADDRESS_MODE GetWrap(size_t wrap);

	void PrepareRenderTargets();
protected:
	virtual void OnInit() override;
	virtual void OnInitThread() override;
	virtual void OnExitThread() override;
	virtual void OnReset() override;
	virtual void ExecCMD(u32 cmd) override;
	virtual void ExecCMD() override;
	virtual void Flip() override;
};

#endif