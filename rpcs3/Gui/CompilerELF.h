#pragma once

#include "Emu/Cell/PPUOpcodes.h"
#include "wx/aui/aui.h"
#include "Loader/ELF64.h"
#include <wx/richtext/richtextctrl.h>

void Write8(wxFile& f, const u8 data);
void Write16(wxFile& f, const u16 data);
void Write32(wxFile& f, const u32 data);
void Write64(wxFile& f, const u64 data);

struct SectionInfo
{
	Elf64_Shdr shdr;
	wxString name;
	Array<u8> code;
	u32 section_num;

	SectionInfo(const wxString& name);
	~SectionInfo();

	void SetDataSize(u32 size, u32 align = 0);
};

struct ProgramInfo
{
	Array<u8> code;
	Elf64_Phdr phdr;
	bool is_preload;

	ProgramInfo()
	{
		is_preload = false;
		code.Clear();
		memset(&phdr, 0, sizeof(Elf64_Phdr));
	}
};

class CompilerELF : public FrameBase
{
	wxAuiManager m_aui_mgr;
	wxStatusBar& m_status_bar;
	bool m_disable_scroll;
	AppConnector m_app_connector;

public:
	CompilerELF(wxWindow* parent);
	~CompilerELF();

	wxTextCtrl* asm_list;
	wxTextCtrl* hex_list;
	wxTextCtrl* err_list;
	
	void MouseWheel(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);

	void OnUpdate(wxCommandEvent& event);
	void OnScroll(wxScrollWinEvent& event);
	void UpdateScroll(bool is_hex, int orient);

	void AnalyzeCode(wxCommandEvent& WXUNUSED(event))
	{
		DoAnalyzeCode(false);
	}

	void CompileCode(wxCommandEvent& WXUNUSED(event))
	{
		DoAnalyzeCode(true);
	}

	void LoadElf(wxCommandEvent& event);
	void LoadElf(const wxString& path);

	void SetTextStyle(const wxString& text, const wxColour& color, bool bold=false);
	void SetOpStyle(const wxString& text, const wxColour& color, bool bold=true);
	void DoAnalyzeCode(bool compile);

	void UpdateStatus(int offset=0);
};

static u32 SetField(u32 src, u32 from, u32 to)
{
	return (src & ((1 << ((to - from) + 1)) - 1)) << (31 - to);
}

static u32 SetField(s32 src, u32 p)
{
	return (src & 0x1) << (31 - p);
}

static u32 ToOpcode(u32 i)	{ return SetField(i, 0, 5); }
static u32 ToRS(u32 i)		{ return SetField(i, 6, 10); }
static u32 ToRD(u32 i)		{ return SetField(i, 6, 10); }
static u32 ToRA(u32 i)		{ return SetField(i, 11, 15); }
static u32 ToRB(u32 i)		{ return SetField(i, 16, 20); }
static u32 ToLL(u32 i)		{ return i & 0x03fffffc; }
static u32 ToAA(u32 i)		{ return SetField(i, 30); }
static u32 ToLK(u32 i)		{ return SetField(i, 31); }
static u32 ToIMM16(s32 i)	{ return SetField(i, 16, 31); }
static u32 ToD(s32 i)		{ return SetField(i, 16, 31); }
static u32 ToDS(s32 i)
{
	if(i < 0) return ToD(i + 1);
	return ToD(i);
}
static u32 ToSYS(u32 i)	{ return SetField(i, 6, 31); }
static u32 ToBF(u32 i)		{ return SetField(i, 6, 10); }
static u32 ToBO(u32 i)		{ return SetField(i, 6, 10); }
static u32 ToBI(u32 i)		{ return SetField(i, 11, 15); }
static u32 ToBD(u32 i)		{ return i & 0xfffc; }
static u32 ToMB(u32 i)		{ return SetField(i, 21, 25); }
static u32 ToME(u32 i)		{ return SetField(i, 26, 30); }
static u32 ToSH(u32 i)		{ return SetField(i, 16, 20); }
static u32 ToRC(u32 i)		{ return SetField(i, 31); }
static u32 ToOE(u32 i)		{ return SetField(i, 21); }
static u32 ToL(u32 i)		{ return SetField(i, 10); }
static u32 ToCRFD(u32 i)	{ return SetField(i, 6, 8); }
static u32 ToTO(u32 i)		{ return SetField(i, 6, 10); }