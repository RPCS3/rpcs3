#include "stdafx.h"
#include "MemoryViewer.h"
#include "Emu/Memory/Memory.h"

MemoryViewerPanel::MemoryViewerPanel(wxWindow* parent) 
	: wxFrame(parent, wxID_ANY, "Memory Viewer", wxDefaultPosition, wxSize(700, 450))
	//wxSYSTEM_MENU | wxRESIZE_BORDER | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN)
{
	exit = false;
	m_addr = 0;
	m_colcount = 16;
	m_rowcount = 16;

	this->SetBackgroundColour(wxColour(240,240,240)); //This fix the ugly background color under Windows
	wxBoxSizer& s_panel = *new wxBoxSizer(wxVERTICAL);

	//Tools
	wxBoxSizer& s_tools = *new wxBoxSizer(wxHORIZONTAL);

	//Tools: Memory Viewer Options
	wxStaticBoxSizer& s_tools_mem = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Memory Viewer Options");

	wxStaticBoxSizer& s_tools_mem_addr = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Address");
	t_addr = new wxTextCtrl(this, wxID_ANY, "00000000", wxDefaultPosition, wxSize(60,-1));
	t_addr->SetMaxLength(8);
	s_tools_mem_addr.Add(t_addr);

	wxStaticBoxSizer& s_tools_mem_bytes = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Bytes");
	sc_bytes = new wxSpinCtrl(this, wxID_ANY, "16", wxDefaultPosition, wxSize(44,-1));
	sc_bytes->SetMax(16);
	sc_bytes->SetMin(1);
	s_tools_mem_bytes.Add(sc_bytes);

	wxStaticBoxSizer& s_tools_mem_buttons = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Control");
	wxButton* b_fprev = new wxButton(this, wxID_ANY, "\u00AB", wxDefaultPosition, wxSize(21, 21));
	wxButton* b_prev  = new wxButton(this, wxID_ANY, "<", wxDefaultPosition, wxSize(21, 21));
	wxButton* b_next  = new wxButton(this, wxID_ANY, ">", wxDefaultPosition, wxSize(21, 21));
	wxButton* b_fnext = new wxButton(this, wxID_ANY, "\u00BB", wxDefaultPosition, wxSize(21, 21));
	s_tools_mem_buttons.Add(b_fprev);
	s_tools_mem_buttons.Add(b_prev);
	s_tools_mem_buttons.Add(b_next);
	s_tools_mem_buttons.Add(b_fnext);

	s_tools_mem.Add(&s_tools_mem_addr);
	s_tools_mem.Add(&s_tools_mem_bytes);
	s_tools_mem.Add(&s_tools_mem_buttons);

	//Tools: Raw Image Preview Options
	wxStaticBoxSizer& s_tools_img = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Raw Image Preview");

	wxStaticBoxSizer& s_tools_img_size = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Size");
	sc_img_size_x = new wxSpinCtrl(this, wxID_ANY, "256", wxDefaultPosition, wxSize(60,-1));
	sc_img_size_y = new wxSpinCtrl(this, wxID_ANY, "256", wxDefaultPosition, wxSize(60,-1));
	s_tools_img_size.Add(sc_img_size_x);
	s_tools_img_size.Add(new wxStaticText(this, wxID_ANY, " x "));
	s_tools_img_size.Add(sc_img_size_y);

	sc_img_size_x->SetMax(8192);
	sc_img_size_y->SetMax(8192);
	sc_img_size_x->SetMin(1);
	sc_img_size_y->SetMin(1);

	wxStaticBoxSizer& s_tools_img_mode = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Mode");
	cbox_img_mode = new wxComboBox(this, wxID_ANY);
	cbox_img_mode->Append("RGB");
	cbox_img_mode->Append("ARGB");
	cbox_img_mode->Append("RGBA");
	s_tools_img_mode.Add(cbox_img_mode);

	s_tools_img.Add(&s_tools_img_size);
	s_tools_img.Add(&s_tools_img_mode);

	//Tools: Run tools
	wxStaticBoxSizer& s_tools_buttons = *new wxStaticBoxSizer(wxVERTICAL, this, "Tools");
	wxButton* b_tools_img_view = new wxButton(this, wxID_ANY, "View\nimage", wxDefaultPosition, wxSize(52,-1));
	s_tools_buttons.Add(b_tools_img_view);

	//Tools: Tools = Memory Viewer Options + Raw Image Preview Options + Buttons
	s_tools.AddSpacer(10);
	s_tools.Add(&s_tools_mem);
	s_tools.AddSpacer(10);
	s_tools.Add(&s_tools_img);
	s_tools.AddSpacer(10);
	s_tools.Add(&s_tools_buttons);
	s_tools.AddSpacer(10);

	//Memory Panel
	wxBoxSizer& s_mem_panel = *new wxBoxSizer(wxHORIZONTAL);
	t_mem_addr  = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxNO_BORDER|wxTE_READONLY);
	t_mem_hex   = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxNO_BORDER|wxTE_READONLY);
	t_mem_ascii = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxNO_BORDER|wxTE_READONLY);
	t_mem_addr->SetMinSize(wxSize(68, 228));
	t_mem_addr->SetForegroundColour(wxColour(75, 135, 150));
	
	t_mem_addr->SetScrollbar(wxVERTICAL, 0, 0, 0);
	t_mem_hex ->SetScrollbar(wxVERTICAL, 0, 0, 0);
	t_mem_ascii->SetScrollbar(wxVERTICAL, 0, 0, 0);
	t_mem_addr ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	t_mem_hex  ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	t_mem_ascii->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	s_mem_panel.AddSpacer(10);
	s_mem_panel.Add(t_mem_addr);
	s_mem_panel.Add(t_mem_hex);
	s_mem_panel.Add(t_mem_ascii);
	s_mem_panel.AddSpacer(10);

	//Memory Panel: Set size of the wxTextCtrl's
	int x, y;
	t_mem_hex->GetTextExtent(wxT("T"), &x, &y);
	t_mem_hex->SetMinSize(wxSize(x * 3*m_colcount + 6, 228));
	t_mem_hex->SetMaxSize(wxSize(x * 3*m_colcount + 6, 228));
	t_mem_ascii->SetMinSize(wxSize(x * m_colcount + 6, 228));
	t_mem_ascii->SetMaxSize(wxSize(x * m_colcount + 6, 228));

	//Merge and display everything
	s_panel.AddSpacer(10);
	s_panel.Add(&s_tools);
	s_panel.AddSpacer(10);
	s_panel.Add(&s_mem_panel, 0, 0, 100);
	s_panel.AddSpacer(10);
	SetSizerAndFit(&s_panel);

	//Events
	//Connect( wxEVT_SIZE, wxSizeEventHandler(MemoryViewerPanel::OnResize) );
	Connect(t_addr->GetId(),   wxEVT_COMMAND_TEXT_ENTER,       wxCommandEventHandler(MemoryViewerPanel::OnChangeToolsAddr) );
	Connect(sc_bytes->GetId(), wxEVT_COMMAND_TEXT_ENTER,       wxCommandEventHandler(MemoryViewerPanel::OnChangeToolsBytes) );
	Connect(sc_bytes->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler(MemoryViewerPanel::OnChangeToolsBytes) );

	Connect(b_prev->GetId(),  wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MemoryViewerPanel::Prev));
	Connect(b_next->GetId(),  wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MemoryViewerPanel::Next));
	Connect(b_fprev->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MemoryViewerPanel::fPrev));
	Connect(b_fnext->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MemoryViewerPanel::fNext));

	t_mem_addr ->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(MemoryViewerPanel::OnScrollMemory), NULL, this);
	t_mem_hex  ->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(MemoryViewerPanel::OnScrollMemory), NULL, this);
	t_mem_ascii->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(MemoryViewerPanel::OnScrollMemory), NULL, this);
	
	//Fill the wxTextCtrl's
	ShowMemory();
};

/*void MemoryViewerPanel::OnResize(wxSizeEvent& event)
{
	const wxSize size(GetClientSize());
	hex_wind->SetSize( size.GetWidth(), size.GetHeight() - 25);
	hex_wind->SetColumnWidth(COL_COUNT, size.GetWidth() - m_colcount - 4);

	event.Skip();
}*/

void MemoryViewerPanel::OnChangeToolsAddr(wxCommandEvent& event)
{
	t_addr->GetValue().ToULong((unsigned long *)&m_addr, 16);
	t_addr->SetValue(wxString::Format("%08x", m_addr));
	ShowMemory();
	event.Skip();
}

void MemoryViewerPanel::OnChangeToolsBytes(wxCommandEvent& event)
{
	m_colcount = sc_bytes->GetValue();

	int x, y;
	t_mem_hex->GetTextExtent(wxT("T"), &x, &y);
	t_mem_hex->SetMinSize(wxSize(x * 3*m_colcount + 6, 228));
	t_mem_hex->SetMaxSize(wxSize(x * 3*m_colcount + 6, 228));
	t_mem_ascii->SetMinSize(wxSize(x * m_colcount + 6, 228));
	t_mem_ascii->SetMaxSize(wxSize(x * m_colcount + 6, 228));
	this->Layout();
	ShowMemory();
	event.Skip();
}

void MemoryViewerPanel::OnScrollMemory(wxMouseEvent& event)
{
	m_addr -= (event.ControlDown() ? m_rowcount : 1) * m_colcount * (event.GetWheelRotation() / event.GetWheelDelta());

	t_addr->SetValue(wxString::Format("%08x", m_addr));
	ShowMemory();
	event.Skip();
}

void MemoryViewerPanel::ShowMemory()
{
	wxString t_mem_addr_str;
	wxString t_mem_hex_str;
	wxString t_mem_ascii_str;

	for(u32 addr = m_addr; addr != m_addr + m_rowcount * m_colcount; addr += m_colcount)
	{
		t_mem_addr_str += wxString::Format("%08x ", addr);
	}

	for(u32 addr = m_addr; addr != m_addr + m_rowcount * m_colcount; addr++)
	{
		if (Memory.IsGoodAddr(addr))
		{
			const u8 rmem = Memory.Read8(addr);
			t_mem_hex_str += wxString::Format("%02x ", rmem);
			const wxString c_rmem = wxString::Format("%c", rmem);
			t_mem_ascii_str += c_rmem.IsEmpty() ? "." : c_rmem;
		}
		else
		{
			t_mem_hex_str += "?? ";
			t_mem_ascii_str += "?";
		}
	}

	t_mem_addr->SetValue(t_mem_addr_str);
	t_mem_hex->SetValue(t_mem_hex_str);
	t_mem_ascii->SetValue(t_mem_ascii_str);
}

void MemoryViewerPanel::Next (wxCommandEvent& WXUNUSED(event)) { m_addr += m_colcount; ShowMemory(); }
void MemoryViewerPanel::Prev (wxCommandEvent& WXUNUSED(event)) { m_addr -= m_colcount; ShowMemory(); }
void MemoryViewerPanel::fNext(wxCommandEvent& WXUNUSED(event)) { m_addr += m_rowcount * m_colcount; ShowMemory(); }
void MemoryViewerPanel::fPrev(wxCommandEvent& WXUNUSED(event)) { m_addr -= m_rowcount * m_colcount; ShowMemory(); }