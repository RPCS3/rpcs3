#include "stdafx.h"
#include "CompilerELF.h"
#include "Emu/Cell/PPUProgramCompiler.h"
using namespace PPU_opcodes;

enum CompilerIDs
{
	id_analyze_code = 0x555,
	id_compile_code,
	id_load_elf,
};

wxFont GetFont(int size)
{
	return wxFont(size, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
}

CompilerELF::CompilerELF(wxWindow* parent)
	: FrameBase(parent, wxID_ANY, "CompilerELF", "", wxSize(640, 680))
	, m_status_bar(*CreateStatusBar())
{
	m_disable_scroll = false;

	m_aui_mgr.SetManagedWindow(this);

	wxMenuBar& menubar(*new wxMenuBar());
	wxMenu& menu_code(*new wxMenu());
	menubar.Append(&menu_code, "Code");

	//menu_code.Append(id_analyze_code, "Analyze");
	menu_code.Append(id_compile_code, "Compile");
	menu_code.Append(id_load_elf, "Load ELF");

	SetMenuBar(&menubar);

	asm_list = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(400, -1),
		wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_RICH2);

	hex_list = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(155, -1),
		wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_READONLY | wxTE_RICH2);

	err_list = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(140, 200),
		wxTE_MULTILINE | wxTE_READONLY);

	const wxSize& client_size = GetClientSize();

	m_aui_mgr.AddPane(asm_list, wxAuiPaneInfo().Name(L"asm_list").CenterPane().PaneBorder(false).CloseButton(false));
	m_aui_mgr.AddPane(hex_list, wxAuiPaneInfo().Name(L"hex_list").Left().PaneBorder(false).CloseButton(false));
	m_aui_mgr.AddPane(err_list, wxAuiPaneInfo().Name(L"err_list").Bottom().PaneBorder(false).CloseButton(false));
	m_aui_mgr.GetPane(L"asm_list").CaptionVisible(false).Show();
	m_aui_mgr.GetPane(L"hex_list").CaptionVisible(false).Show();
	m_aui_mgr.GetPane(L"err_list").Caption("Errors").Show();
	m_aui_mgr.Update();

	FrameBase::LoadInfo();

	Connect(asm_list->GetId(), wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(CompilerELF::OnUpdate));
	Connect(id_analyze_code,   wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CompilerELF::AnalyzeCode));
	Connect(id_compile_code,   wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CompilerELF::CompileCode));
	Connect(id_load_elf,       wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CompilerELF::LoadElf));

	asm_list->SetFont(wxFont(-1, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	hex_list->SetFont(wxFont(-1, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	m_app_connector.Connect(wxEVT_SCROLLWIN_TOP,          wxScrollWinEventHandler(CompilerELF::OnScroll), (wxObject*)0, this);
	m_app_connector.Connect(wxEVT_SCROLLWIN_BOTTOM,       wxScrollWinEventHandler(CompilerELF::OnScroll), (wxObject*)0, this);
	m_app_connector.Connect(wxEVT_SCROLLWIN_LINEUP,       wxScrollWinEventHandler(CompilerELF::OnScroll), (wxObject*)0, this);
	m_app_connector.Connect(wxEVT_SCROLLWIN_LINEDOWN,     wxScrollWinEventHandler(CompilerELF::OnScroll), (wxObject*)0, this);
	m_app_connector.Connect(wxEVT_SCROLLWIN_THUMBTRACK,   wxScrollWinEventHandler(CompilerELF::OnScroll), (wxObject*)0, this);
	m_app_connector.Connect(wxEVT_SCROLLWIN_THUMBRELEASE, wxScrollWinEventHandler(CompilerELF::OnScroll), (wxObject*)0, this);

	m_app_connector.Connect(asm_list->GetId(), wxEVT_MOUSEWHEEL, wxMouseEventHandler(CompilerELF::MouseWheel), (wxObject*)0, this);
	m_app_connector.Connect(hex_list->GetId(), wxEVT_MOUSEWHEEL, wxMouseEventHandler(CompilerELF::MouseWheel), (wxObject*)0, this);

	m_app_connector.Connect(asm_list->GetId(), wxEVT_KEY_DOWN, wxKeyEventHandler(CompilerELF::OnKeyDown), (wxObject*)0, this);
	m_app_connector.Connect(hex_list->GetId(), wxEVT_KEY_DOWN, wxKeyEventHandler(CompilerELF::OnKeyDown), (wxObject*)0, this);

	asm_list->WriteText(
		".int [sys_tty_write, 0x193]\n"
		".int [sys_process_exit, 0x003]\n"
		".string [str, \"Hello World!\\n\"]\n"
		".strlen [str_len, str]\n"
		".buf [ret, 8]\n"
		"\n"
		".srr [str_hi, str, 16]\n"
		".and [str_lo, str, 0xffff]\n"
		"\n"
		".srr [ret_hi, str, 16]\n"
		".and [ret_lo, str, 0xffff]\n"
		"\n"
		"	addi	r3, r0, 0\n"
		"	addi 	r4, r0, str_lo\n"
		"	oris 	r4, r4, str_hi\n"
		"	addi 	r5, r0, str_len\n"
		"	addi 	r6, r0, ret_lo\n"
		"	oris 	r6, r6, ret_hi\n"
		"	addi	r11, r0, sys_tty_write\n"
		"	sc	2\n"
		"	cmpi 	cr7, 0, r3, 0\n"
		"	bc 	0x04, 30, exit_err, 0, 0\n"
		"\n"
		"exit_ok:\n"
		"	addi 	r3, r0, 0\n"
		"	b 	exit, 0, 0\n"
		"\n"
		"exit_err:\n"
		".string [str, \"error.\\n\"]\n"
		".srr [str_hi, str, 16]\n"
		".and [str_lo, str, 0xffff]\n"
		".strlen [str_len, str]\n"
		".int [written_len_lo, fd_lo]\n"
		".int [written_len_hi, fd_hi]\n"
		"	addi 	r3, r0, 1\n"
		"	addi 	r4, r0, str_lo\n"
		"	oris 	r4, r4, str_hi\n"
		"	addi 	r5, r0, str_len\n"
		"	addi 	r6, r0, written_len_lo\n"
		"	oris 	r6, r6, written_len_hi\n"
		"	addi	r11, r0, sys_tty_write\n"
		"	sc 2\n"
		"	addi 	r3, r0, 1\n"
		"\n"
		"exit:\n"
		"	addi	r11, r0, sys_process_exit\n"
		"	sc 2\n"
		"	addi 	r3, r0, 1\n"
		"	b 	exit, 0, 0\n"
	);

#ifdef _WIN32
	::SendMessage((HWND)hex_list->GetHWND(), WM_VSCROLL, SB_BOTTOM, 0);
	::SendMessage((HWND)asm_list->GetHWND(), WM_VSCROLL, SB_BOTTOM, 0);
#endif
}

CompilerELF::~CompilerELF()
{
	m_aui_mgr.UnInit();
}

void CompilerELF::MouseWheel(wxMouseEvent& event)
{
	int value = (event.m_wheelRotation / event.m_wheelDelta);

	wxScrollWinEvent scrool_event;

	scrool_event.SetEventType(value > 0 ? wxEVT_SCROLLWIN_LINEUP : wxEVT_SCROLLWIN_LINEDOWN);
	scrool_event.SetOrientation(wxVERTICAL);

	if(value < 0) value = -value;

	value *= event.m_linesPerAction;

	while(value--)
	{
		scrool_event.SetEventObject(asm_list);
		OnScroll(scrool_event);
		scrool_event.SetEventObject(hex_list);
		OnScroll(scrool_event);
	}
}

void CompilerELF::UpdateStatus(int offset)
{
	long line;

	if(asm_list->PositionToXY(asm_list->GetInsertionPoint() - 2, nullptr, &line))
	{
		m_status_bar.SetStatusText(wxString::Format("line %d", line), 0);
		return;
	}

	m_status_bar.SetStatusText(wxEmptyString, 0);
}

void CompilerELF::OnKeyDown(wxKeyEvent& event)
{
	wxScrollWinEvent scrool_event;
	scrool_event.SetOrientation(wxVERTICAL);

	switch(event.GetKeyCode())
	{
	case WXK_RETURN: UpdateStatus( 1); break;
	case WXK_UP:     UpdateStatus(-1); break;
	case WXK_DOWN:   UpdateStatus( 1); break;

	case WXK_LEFT:
	case WXK_RIGHT:  UpdateStatus(); break;

	case WXK_PAGEUP:
		scrool_event.SetEventType(wxEVT_SCROLLWIN_PAGEUP);
		scrool_event.SetEventObject(asm_list);
		OnScroll(scrool_event);
		scrool_event.SetEventObject(hex_list);
		OnScroll(scrool_event);
	return;

	case WXK_PAGEDOWN:
		scrool_event.SetEventType(wxEVT_SCROLLWIN_PAGEDOWN);
		scrool_event.SetEventObject(asm_list);
		OnScroll(scrool_event);
		scrool_event.SetEventObject(hex_list);
		OnScroll(scrool_event);
	return;
	}

	event.Skip();
}

void CompilerELF::OnUpdate(wxCommandEvent& event)
{
	UpdateStatus();
	DoAnalyzeCode(false);

	return;
	asm_list->Freeze();
	asm_list->SetStyle(0, asm_list->GetValue().Len(), wxTextAttr("Black"));

	/*
	for(u32 i=0; i<instr_count; ++i)
	{
		SetOpStyle(m_instructions[i].name, "Blue");
	}
	*/
	
	SetOpStyle(".int", "Blue");
	SetOpStyle(".string", "Blue");
	SetOpStyle(".strlen", "Blue");
	SetOpStyle(".buf", "Blue");
	SetOpStyle(".srl", "Blue");
	SetOpStyle(".srr", "Blue");
	SetOpStyle(".mul", "Blue");
	SetOpStyle(".div", "Blue");
	SetOpStyle(".add", "Blue");
	SetOpStyle(".sub", "Blue");
	SetOpStyle(".and", "Blue");
	SetOpStyle(".or", "Blue");
	SetOpStyle(".xor", "Blue");
	SetOpStyle(".not", "Blue");
	SetOpStyle(".nor", "Blue");

	SetTextStyle("[", "Red");
	SetTextStyle("]", "Red");
	SetTextStyle(":", "Red");
	SetTextStyle(",", "Red");

	for(int p=0; (p = asm_list->GetValue().find('#', p)) >= 0;)
	{
		const int from = p++;
		p = asm_list->GetValue().find('\n', p);

		if(p < 0) p = asm_list->GetValue().Len();

		asm_list->SetStyle(from, p, wxTextAttr(0x009900));
	}
	
	for(int p=0; (p = asm_list->GetValue().find('"', p)) >= 0;)
	{
		const int from = p++;
		const int p1 = asm_list->GetValue().find('\n', p);

		int p2 = p;
		for(;;)
		{
			p2 = asm_list->GetValue().find('"', p2);
			if(p2 == -1) break;

			if(asm_list->GetValue()[p2 - 1] == '\\')
			{
				if(p2 > 2 && asm_list->GetValue()[p2 - 2] == '\\') break;

				p2++;
				continue;
			}
			break;
		}

		if(p1 < 0 && p2 < 0)
		{
			p = asm_list->GetValue().Len();
		}
		else if(p1 >= 0 && p2 < 0)
		{
			p = p1;
		}
		else if(p2 >= 0 && p1 < 0)
		{
			p = p2 + 1;
		}
		else
		{
			p = p1 > p2 ? p2 + 1 : p1;
		}

		asm_list->SetStyle(from, p, wxTextAttr(0x000099));
	}

	asm_list->Thaw();

	UpdateScroll(true, wxVERTICAL);
}

void CompilerELF::OnScroll(wxScrollWinEvent& event)
{
	if(!m_aui_mgr.GetManagedWindow()) return;

	const int id = event.GetEventObject() ? ((wxScrollBar*)event.GetEventObject())->GetId() : -1;

	wxTextCtrl* src = NULL;
	wxTextCtrl* dst = NULL;

	if(id == hex_list->GetId())
	{
		src = hex_list;
		dst = asm_list;
	}
	else if(id == asm_list->GetId())
	{
		src = asm_list;
		dst = hex_list;
	}

#ifdef _WIN32
	if(!m_disable_scroll && src && dst && event.GetOrientation() == wxVERTICAL)
	{
		s64 kind = -1;

		if(event.GetEventType() == wxEVT_SCROLLWIN_TOP)
		{
			kind = SB_TOP;
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_BOTTOM)
		{
			kind = SB_BOTTOM;
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
		{
			kind = SB_LINEUP;
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
		{
			kind = SB_LINEDOWN;
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
		{
			kind = SB_PAGEUP;
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
		{
			kind = SB_PAGEDOWN;
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK)
		{
			kind = MAKEWPARAM(SB_THUMBTRACK, event.GetPosition());
			//dst->SetScrollPos(event.GetOrientation(), event.GetPosition());
		}
		else if(event.GetEventType() == wxEVT_SCROLLWIN_THUMBRELEASE)
		{
			kind = MAKEWPARAM(SB_THUMBPOSITION, event.GetPosition());
			//dst->SetScrollPos(event.GetOrientation(), event.GetPosition());
		}

		if(kind >= 0)
		{
			m_disable_scroll = true;
			::SendMessage((HWND)dst->GetHWND(), (event.GetOrientation() == wxVERTICAL ? WM_VSCROLL : WM_HSCROLL), kind, 0);
			m_disable_scroll = false;
		}
	}
#endif

	event.Skip();
}

void CompilerELF::UpdateScroll(bool is_hex, int orient)
{
	wxScrollWinEvent event;

	event.SetEventType(wxEVT_SCROLLWIN_THUMBRELEASE);
	event.SetEventObject(is_hex ? asm_list : hex_list);
	event.SetPosition(is_hex ? asm_list->GetScrollPos(orient) : hex_list->GetScrollPos(orient));
	event.SetOrientation(orient);

	OnScroll(event);
}

void CompilerELF::LoadElf(wxCommandEvent& event)
{
	wxFileDialog ctrl(this, L"Select ELF", wxEmptyString, wxEmptyString,
		"Elf Files (*.elf, *.bin)|*.elf;*.bin|"
		"All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if(ctrl.ShowModal() == wxID_CANCEL) return;
	LoadElf(fmt::ToUTF8(ctrl.GetPath()));
}

#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/PPUDecoder.h"

void CompilerELF::LoadElf(const std::string& path)
{
}

void CompilerELF::SetTextStyle(const std::string& text, const wxColour& color, bool bold)
{
	for(int p=0; (p = fmt::ToUTF8(asm_list->GetValue()).find(text, p)) !=std::string::npos; p += text.length())
	{
		asm_list->SetStyle(p, p + text.length(), wxTextAttr(color, wxNullColour, 
			wxFont(-1, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL)));
	}
}

void CompilerELF::SetOpStyle(const std::string& text, const wxColour& color, bool bold)
{
	/*
	for(int p=0; (p = FindOp(asm_list->GetValue(), text, p)) >= 0; p += text.Len())
	{
		asm_list->SetStyle(p, p + text.Len(), wxTextAttr(color, wxNullColour, 
			wxFont(-1, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL)));
	}
	*/
}

void CompilerELF::DoAnalyzeCode(bool compile)
{
	CompilePPUProgram(fmt::ToUTF8(asm_list->GetValue()), "compiled.elf", asm_list, hex_list, err_list, !compile).Compile();
}
