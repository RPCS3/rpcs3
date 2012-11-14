#include "stdafx.h"
#include "MemoryViewer.h"
#include "Emu/Memory/Memory.h"

MemoryViewerPanel::MemoryViewerPanel(wxWindow* parent) 
	: FrameBase(parent, wxID_ANY, L"Memory Viewer", wxEmptyString, wxSize(700, 450), wxDefaultPosition,
	wxSYSTEM_MENU | wxRESIZE_BORDER | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN)
{
	exit = false;

	m_PC = 0;

	wxBoxSizer& s_panel( *new wxBoxSizer(wxVERTICAL) );
	wxBoxSizer& s_b_panel( *new wxBoxSizer(wxHORIZONTAL) );

	hex_wind = new wxListView(this);

	for(uint i=0; i<COL_COUNT; ++i)
	{
		hex_wind->InsertColumn(i, wxString::Format("%d", i));
		hex_wind->SetColumnWidth(i, 28);
	}

	hex_wind->InsertColumn(COL_COUNT, wxEmptyString);
	hex_wind->SetColumnWidth(COL_COUNT, 20);

	m_colsize = 0;

	for(uint i=0; i<COL_COUNT; ++i)
	{
		m_colsize += hex_wind->GetColumnWidth(i);
	}

	SetMinSize(wxSize(m_colsize + hex_wind->GetColumnWidth(COL_COUNT), 50));

	for(uint i=0; i<LINE_COUNT; ++i) hex_wind->InsertItem(i, -1);

	wxButton& b_fprev	= *new wxButton(this, wxID_ANY, L"<<");
	wxButton& b_prev	= *new wxButton(this, wxID_ANY, L"<");
	wxButton& b_next	= *new wxButton(this, wxID_ANY, L">");
	wxButton& b_fnext	= *new wxButton(this, wxID_ANY, L">>");

	s_b_panel.Add(&b_fprev);
	s_b_panel.Add(&b_prev);
	s_b_panel.AddSpacer(5);
	s_b_panel.Add(&b_next);
	s_b_panel.Add(&b_fnext);

	s_panel.Add(&s_b_panel);
	s_panel.Add(hex_wind);

	SetSizerAndFit( &s_panel );

	Connect( wxEVT_SIZE, wxSizeEventHandler(MemoryViewerPanel::OnResize) );

	Connect(b_prev.GetId(),	 wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MemoryViewerPanel::Prev));
	Connect(b_next.GetId(),  wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MemoryViewerPanel::Next));
	Connect(b_fprev.GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MemoryViewerPanel::fPrev));
	Connect(b_fnext.GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MemoryViewerPanel::fNext));
};

void MemoryViewerPanel::OnResize(wxSizeEvent& event)
{
	const wxSize size(GetClientSize());
	hex_wind->SetSize( size.GetWidth(), size.GetHeight() - 25);
	hex_wind->SetColumnWidth(COL_COUNT, size.GetWidth() - m_colsize - 4);

	event.Skip();
}

void MemoryViewerPanel::ShowPC()
{
	uint pc = m_PC;

	for(uint line=0; line < LINE_COUNT; ++line)
	{
		wxString char_col = wxEmptyString;

		for(uint item=0; item < COL_COUNT; ++item)
		{
			const u8 rmem = Memory.Read8(pc++);

			hex_wind->SetItem(line, item, wxString::Format("%02X", rmem));

			const wxString c_rmem = wxString::Format("%c", rmem);
			char_col += c_rmem.IsEmpty() ? "." : c_rmem;
		}

		hex_wind->SetItem(line, COL_COUNT, char_col);
	}
}

void MemoryViewerPanel::Next (wxCommandEvent& WXUNUSED(event)) { m_PC += COL_COUNT; ShowPC(); }
void MemoryViewerPanel::Prev (wxCommandEvent& WXUNUSED(event)) { m_PC -= COL_COUNT; ShowPC(); }
void MemoryViewerPanel::fNext(wxCommandEvent& WXUNUSED(event)) { m_PC += LINE_COUNT * COL_COUNT; ShowPC(); }
void MemoryViewerPanel::fPrev(wxCommandEvent& WXUNUSED(event)) { m_PC -= LINE_COUNT * COL_COUNT; ShowPC(); }