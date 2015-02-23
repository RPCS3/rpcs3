#include "stdafx_gui.h"

#include "CgDisasm.h"
#include "Emu/System.h"
#include "Emu/RSX/CgBinaryProgram.h"

BEGIN_EVENT_TABLE(CgDisasm, wxFrame)
	EVT_SIZE(CgDisasm::OnSize)
END_EVENT_TABLE()

CgDisasm::CgDisasm(wxWindow* parent)
	: wxFrame(parent, wxID_ANY, "Cg Disasm", wxDefaultPosition, wxSize(640, 480))
{
	wxMenuBar* menubar = new wxMenuBar();

	wxMenu* menu_general = new wxMenu();
	menubar->Append(menu_general, "&Open");
	menu_general->Append(id_open_file, "Open &Cg binary program");

	wxNotebook* nb_cg = new wxNotebook(this, wxID_ANY);
	wxPanel* p_cg_disasm = new wxPanel(nb_cg, wxID_ANY);
	wxPanel* p_glsl_shader = new wxPanel(nb_cg, wxID_ANY);

	nb_cg->AddPage(p_cg_disasm, wxT("ASM"));
	nb_cg->AddPage(p_glsl_shader, wxT("GLSL"));

	m_disasm_text = new wxTextCtrl(p_cg_disasm, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(620, 395), wxTE_MULTILINE | wxNO_BORDER | wxTE_READONLY | wxTE_RICH2);
	m_glsl_text = new wxTextCtrl(p_glsl_shader, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(620, 395), wxTE_MULTILINE | wxNO_BORDER | wxTE_READONLY | wxTE_RICH2);

	SetMenuBar(menubar);

	m_disasm_text->Bind(wxEVT_RIGHT_DOWN, &CgDisasm::OnRightClick, this);
	m_glsl_text->Bind(wxEVT_RIGHT_DOWN, &CgDisasm::OnRightClick, this);

	Bind(wxEVT_MENU, &CgDisasm::OpenCg, this, id_open_file);
	Bind(wxEVT_MENU, &CgDisasm::OnContextMenu, this, id_clear);
}

void CgDisasm::OpenCg(wxCommandEvent& event)
{
	wxFileDialog ctrl(this, L"Select Cg program object", wxEmptyString, wxEmptyString,
		"Cg program objects (*.fpo;*.vpo)|*.fpo;*.vpo"
		"|All files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (ctrl.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	CgBinaryDisasm disasm(fmt::ToUTF8(ctrl.GetPath()));
	disasm.BuildShaderBody();
	*m_disasm_text << disasm.GetArbShader();
	*m_glsl_text << disasm.GetGlslShader();
}

void CgDisasm::OnSize(wxSizeEvent& event)
{
	m_disasm_text->SetSize(GetSize().x - 20, GetSize().y - 85);
	m_glsl_text->SetSize(GetSize().x - 20, GetSize().y - 85);
	event.Skip();
}

void CgDisasm::OnRightClick(wxMouseEvent& event)
{
	wxMenu* menu = new wxMenu();
	menu->Append(id_clear, "&Clear");
	PopupMenu(menu);
}

void CgDisasm::OnContextMenu(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case id_clear:
		m_disasm_text->Clear();
		m_glsl_text->Clear();
		break;
	default:
		event.Skip();
	}
}
