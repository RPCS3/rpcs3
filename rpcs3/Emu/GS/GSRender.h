#pragma once
#include "Emu/GS/GCM.h"
#include "Emu/SysCalls/Callback.h"
#include "rpcs3.h"

enum Method
{
	CELL_GCM_METHOD_FLAG_NON_INCREMENT	= 0x40000000,
	CELL_GCM_METHOD_FLAG_JUMP			= 0x20000000,
	CELL_GCM_METHOD_FLAG_CALL			= 0x00000002,
	CELL_GCM_METHOD_FLAG_RETURN			= 0x00020000,
};


wxSize AspectRatio(wxSize rs, const wxSize as);

class GSFrame : public wxFrame
{
protected:
	GSFrame(wxWindow* parent, const wxString& title);

	virtual void SetViewport(int x, int y, u32 w, u32 h) {}
	virtual void OnPaint(wxPaintEvent& event);
	virtual void OnClose(wxCloseEvent& event);

	//virtual void OnSize(wxSizeEvent&);

	void OnKeyDown(wxKeyEvent& event);
	void OnFullScreen();

public:
	void OnLeftDclick(wxMouseEvent&)
	{
		OnFullScreen();
	}

	//void SetSize(int width, int height);

private:
	DECLARE_EVENT_TABLE();
};

struct CellGcmSurface
{
	u8 type;
	u8 antialias;

	u8 color_format;
	u8 color_target;
	u8 color_location[4];
	u32 color_offset[4];
	u32 color_pitch[4];

	u8 depth_format;
	u8 depth_location;
	u16 pad;
	u32 depth_offset;
	u32 depth_pitch;

	u16 width;
	u16 height;
	u16 x;
	u16 y;
};

struct CellGcmReportData
{
	u64 timer;
	u32 value;
	u32 pad;
};

struct CellGcmZcullInfo
{
	u32 region;
	u32 size;
	u32 start;
	u32 offset;
	u32 status0;
	u32 status1;
};

struct CellGcmTileInfo
{
	u32 tile;
	u32 limit;
	u32 pitch;
	u32 format;
};

struct GcmZcullInfo
{
	u32 m_offset;
	u32 m_width;
	u32 m_height;
	u32 m_cullStart;
	u32 m_zFormat;
	u32 m_aaFormat;
	u32 m_zCullDir;
	u32 m_zCullFormat;
	u32 m_sFunc;
	u32 m_sRef;
	u32 m_sMask;
	bool m_binded;

	GcmZcullInfo()
	{
		memset(this, 0, sizeof(*this));
	}
};

struct GcmTileInfo
{
	u8 m_location;
	u32 m_offset;
	u32 m_size;
	u32 m_pitch;
	u8 m_comp;
	u16 m_base;
	u8 m_bank;
	bool m_binded;

	GcmTileInfo()
	{
		memset(this, 0, sizeof(*this));
	}

	CellGcmTileInfo Pack()
	{
		CellGcmTileInfo ret;

		re(ret.tile, (m_location + 1) | (m_bank << 4) | ((m_offset / 0x10000) << 16) | (m_location << 31));
		re(ret.limit, ((m_offset + m_size - 1) / 0x10000) << 16 | (m_location << 31));
		re(ret.pitch, (m_pitch / 0x100) << 8);
		re(ret.format, m_base | ((m_base + ((m_size - 1) / 0x10000)) << 13) | (m_comp << 26) | (1 << 30));

		return ret;
	}
};

static const int g_tiles_count = 15;

struct GSRender
{
	u32 m_ioAddress, m_ioSize, m_ctrlAddress;
	CellGcmControl* m_ctrl;
	wxCriticalSection m_cs_main;
	wxSemaphore m_sem_flush;
	wxSemaphore m_sem_flip;
	int m_flip_status;
	int m_flip_mode;
	volatile bool m_draw;
	Callback m_flip_handler;

	GcmTileInfo m_tiles[g_tiles_count];

	u32 m_tiles_addr;
	u32 m_zculls_addr;
	u32 m_gcm_buffers_addr;
	u32 m_gcm_buffers_count;
	u32 m_gcm_current_buffer;
	u32 m_ctxt_addr;
	u32 m_report_main_addr;

	u32 m_local_mem_addr, m_main_mem_addr;
	Array<MemInfo> m_main_mem_info;

	GSRender();

	virtual void Init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress)=0;
	virtual void Draw()=0;
	virtual void Close()=0;

	u32 GetAddress(u32 offset, u8 location);
};

enum GSLockType
{
	GS_LOCK_NOT_WAIT,
	GS_LOCK_WAIT_FLUSH,
	GS_LOCK_WAIT_FLIP,
};

struct GSLock
{
private:
	GSRender& m_renderer;
	GSLockType m_type;

public:
	GSLock(GSRender& renderer, GSLockType type)
		: m_renderer(renderer)
		, m_type(type)
	{
		switch(m_type)
		{
		case GS_LOCK_NOT_WAIT: m_renderer.m_cs_main.Enter(); break;
		case GS_LOCK_WAIT_FLUSH: m_renderer.m_sem_flush.Wait(); break;
		case GS_LOCK_WAIT_FLIP: m_renderer.m_sem_flip.Wait(); break;
		}
	}

	~GSLock()
	{
		switch(m_type)
		{
		case GS_LOCK_NOT_WAIT: m_renderer.m_cs_main.Leave(); break;
		case GS_LOCK_WAIT_FLUSH: m_renderer.m_sem_flush.Post(); break;
		case GS_LOCK_WAIT_FLIP: m_renderer.m_sem_flip.Post(); break;
		}
	}
};

struct GSLockCurrent : GSLock
{
	GSLockCurrent(GSLockType type);
};