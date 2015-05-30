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

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")

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


class D3D12GSRender : public GSRender
{
private:
	// Copy of RTT to be used as texture
	std::unordered_map<u32, Microsoft::WRL::ComPtr<ID3D12Resource> > m_texturesRTTs;

	std::unordered_map<u32, ID3D12Resource*> m_texturesCache;
	//  std::vector<PostDrawObj> m_post_draw_objs;

	PipelineStateObjectCache m_cachePSO;
	ID3D12PipelineState *m_PSO;
	ID3D12RootSignature *m_rootSignature;

	ID3D12PipelineState *m_convertPSO;
	ID3D12RootSignature *m_convertRootSignature;

	struct ResourceStorage
	{
		ID3D12CommandAllocator *m_commandAllocator;
		ID3D12CommandAllocator *m_downloadCommandAllocator;
		std::list<ID3D12GraphicsCommandList *> m_inflightCommandList;

		std::vector<ID3D12Resource *> m_inflightResources;

		// Vertex storage
		size_t m_vertexIndexBuffersHeapFreeSpace;
		ID3D12Heap *m_vertexIndexBuffersHeap;

		// Constants storage
		ID3D12Heap *m_constantsBuffersHeap;
		size_t m_constantsBuffersHeapFreeSpace;
		ID3D12DescriptorHeap *m_constantsBufferDescriptorsHeap;
		size_t m_constantsBufferIndex;
		ID3D12DescriptorHeap *m_scaleOffsetDescriptorHeap;
		size_t m_currentScaleOffsetBufferIndex;

		// Texture storage
		ID3D12CommandAllocator *m_textureUploadCommandAllocator;
		ID3D12Heap *m_uploadTextureHeap, *m_textureStorage;
		size_t m_currentStorageOffset;
		ID3D12DescriptorHeap *m_textureDescriptorsHeap;
		ID3D12DescriptorHeap *m_samplerDescriptorHeap;
		size_t m_currentTextureIndex;

		void Reset();
		void Init(ID3D12Device *device);
		void Release();
	};

	ResourceStorage m_perFrameStorage;

	struct UAVHeap
	{
		ID3D12Heap *m_heap;
		std::atomic<int> m_putPos, // Start of free space
			m_getPos; // End of free space
	};

	UAVHeap m_UAVHeap;

	struct ReadbackHeap
	{
		ID3D12Heap *m_heap;
		std::atomic<int> m_putPos, // Start of free space
			m_getPos; // End of free space
	};

	ReadbackHeap m_readbackResources;

	bool m_forcedIndexBuffer;
	size_t indexCount;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_IASet;
	D3D12RenderTargetSets *m_fbo;
	ID3D12Device* m_device;
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
	float *vertexConstantShadowCopy;

	D3D12GSRender();
	virtual ~D3D12GSRender();

	virtual void semaphorePGRAPHBackendRelease(u32 offset, u32 value) override;
	virtual void semaphorePFIFOAcquire(u32 offset, u32 value) override;

private:
	ID3D12Resource *writeColorBuffer(ID3D12Resource *RTT, ID3D12GraphicsCommandList *cmdlist);
	virtual void Close() override;

	bool LoadProgram();
	std::pair<std::vector<D3D12_VERTEX_BUFFER_VIEW>, D3D12_INDEX_BUFFER_VIEW> EnableVertexData(bool indexed_draw = false);
	void setScaleOffset();
	void FillVertexShaderConstantsBuffer();
	void FillPixelShaderConstantsBuffer();
	/**
	 * returns the number of texture uploaded
	 */
	size_t UploadTextures();
	size_t GetMaxAniso(size_t aniso);
	D3D12_TEXTURE_ADDRESS_MODE GetWrap(size_t wrap);

	/*void DisableVertexData();

		void WriteBuffers();
		void WriteColorBuffers();
		void WriteColorBufferA();
		void WriteColorBufferB();
		void WriteColorBufferC();
		void WriteColorBufferD();

		void DrawObjects();*/
	void InitDrawBuffers();
	void WriteDepthBuffer();
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