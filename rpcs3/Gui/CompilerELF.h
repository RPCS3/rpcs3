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