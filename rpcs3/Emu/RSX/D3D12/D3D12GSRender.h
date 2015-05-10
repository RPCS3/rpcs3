#pragma once

#ifdef _WIN32
#include <d3d12.h>
#include "rpcs3/Ini.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Utilities/File.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/RSX/GSRender.h"

#include "D3D12RenderTargetSets.h"

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

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

};

typedef GSFrameBase2*(*GetGSFrameCb2)();

void SetGetD3DGSFrameCallback(GetGSFrameCb2 value);


class D3D12GSRender //TODO: find out why this used to inherit from wxWindow
	: //public wxWindow
	/*,*/ public GSRender
{
private:
	//  std::vector<u8> m_vdata;
	//  std::vector<PostDrawObj> m_post_draw_objs;

	//  GLProgram m_program;
	int m_fp_buf_num;
	int m_vp_buf_num;
	//  GLProgramBuffer m_prog_buffer;

	//  GLFragmentProgram m_fragment_prog;
	//  GLVertexProgram m_vertex_prog;

	//  GLTexture m_gl_textures[m_textures_count];
	//  GLTexture m_gl_vertex_textures[m_textures_count];

	//  GLvao m_vao;
	//  GLvbo m_vbo;
	//  GLrbo m_rbo;
	D3D12RenderTargetSets m_fbo;
	ID3D12Device* m_device;
	ID3D12CommandQueue *m_commandQueueCopy;
	ID3D12CommandQueue *m_commandQueueGraphic;

	size_t m_lastWidth, m_lastHeight, m_lastDepth;

	void* m_context;

public:
	//  GSFrameBase* m_frame;
	u32 m_draw_frames;
	u32 m_skip_frames;

	D3D12GSRender();
	virtual ~D3D12GSRender();

private:
	virtual void Close() override;
	/*  void EnableVertexData(bool indexed_draw = false);
		void DisableVertexData();
		void InitVertexData();
		void InitFragmentData();

		void Enable(bool enable, const u32 cap);

		bool LoadProgram();
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