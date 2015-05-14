#pragma once
#if defined(DX12_SUPPORT)

#include <d3d12.h>
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
};

typedef GSFrameBase2*(*GetGSFrameCb2)();

void SetGetD3DGSFrameCallback(GetGSFrameCb2 value);


class D3D12GSRender : public GSRender
{
private:
	size_t m_vertexBufferSize[32];
	//  std::vector<PostDrawObj> m_post_draw_objs;

	PipelineStateObjectCache m_cachePSO;
	ID3D12PipelineState *m_PSO;
	ID3D12RootSignature *m_rootSignature;

	//  GLTexture m_gl_textures[m_textures_count];
	//  GLTexture m_gl_vertex_textures[m_textures_count];

	ID3D12Resource *m_indexBuffer, *m_vertexBuffer[m_vertex_count];
	ID3D12Resource *m_constantsVertexBuffer, *m_constantsFragmentBuffer;
	ID3D12DescriptorHeap *m_constantsBufferDescriptorsHeap;
	size_t m_constantsBufferSize, m_constantsBufferIndex;

	ID3D12Resource *m_scaleOffsetBuffer;
	ID3D12DescriptorHeap *m_scaleOffsetDescriptorHeap;
	size_t m_currentScaleOffsetBufferIndex;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_IASet;
	D3D12RenderTargetSets *m_fbo;
	ID3D12Device* m_device;
	ID3D12CommandQueue *m_commandQueueCopy;
	ID3D12CommandQueue *m_commandQueueGraphic;
	ID3D12CommandAllocator *m_commandAllocator;
	std::list<ID3D12GraphicsCommandList *> m_inflightCommandList;
	struct IDXGISwapChain3 *m_swapChain;
	ID3D12Resource* m_backBuffer[2];

	ID3D12DescriptorHeap *m_backbufferAsRendertarget[2];

	size_t m_lastWidth, m_lastHeight, m_lastDepth;
public:
	GSFrameBase2 *m_frame;
	u32 m_draw_frames;
	u32 m_skip_frames;

	D3D12GSRender();
	virtual ~D3D12GSRender();

private:
	virtual void Close() override;

	bool LoadProgram();
	void EnableVertexData(bool indexed_draw = false);
	void setScaleOffset();
	void FillVertexShaderConstantsBuffer();
	void FillPixelShaderConstantsBuffer();
	/*void DisableVertexData();

		void WriteBuffers();
		void WriteDepthBuffer();
		void WriteColorBuffers();
		void WriteColorBufferA();
		void WriteColorBufferB();
		void WriteColorBufferC();
		void WriteColorBufferD();

		void DrawObjects();*/
	void InitDrawBuffers();

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