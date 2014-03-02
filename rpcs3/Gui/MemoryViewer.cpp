#include "stdafx.h"
#include "MemoryViewer.h"

MemoryViewerPanel::MemoryViewerPanel(wxWindow* parent) 
	: wxFrame(parent, wxID_ANY, "Memory Viewer", wxDefaultPosition, wxSize(700, 450))
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
	sc_bytes->SetRange(1, 16);
	s_tools_mem_bytes.Add(sc_bytes);

	wxStaticBoxSizer& s_tools_mem_buttons = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Control");
	wxButton* b_fprev = new wxButton(this, wxID_ANY, "<<", wxDefaultPosition, wxSize(21, 21));
	wxButton* b_prev  = new wxButton(this, wxID_ANY, "<", wxDefaultPosition, wxSize(21, 21));
	wxButton* b_next  = new wxButton(this, wxID_ANY, ">", wxDefaultPosition, wxSize(21, 21));
	wxButton* b_fnext = new wxButton(this, wxID_ANY, ">>", wxDefaultPosition, wxSize(21, 21));
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

	sc_img_size_x->SetRange(1, 8192);
	sc_img_size_y->SetRange(1, 8192);

	wxStaticBoxSizer& s_tools_img_mode = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Mode");
	cbox_img_mode = new wxComboBox(this, wxID_ANY);
	cbox_img_mode->Append("RGB");
	cbox_img_mode->Append("ARGB");
	cbox_img_mode->Append("RGBA");
	cbox_img_mode->Append("ABGR");
	cbox_img_mode->Select(1); //ARGB
	s_tools_img_mode.Add(cbox_img_mode);

	s_tools_img.Add(&s_tools_img_size);
	s_tools_img.Add(&s_tools_img_mode);

	//Tools: Run tools
	wxStaticBoxSizer& s_tools_buttons = *new wxStaticBoxSizer(wxVERTICAL, this, "Tools");
	wxButton* b_img = new wxButton(this, wxID_ANY, "View\nimage", wxDefaultPosition, wxSize(52,-1));
	s_tools_buttons.Add(b_img);

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
	Connect(t_addr->GetId(),   wxEVT_COMMAND_TEXT_ENTER,       wxCommandEventHandler(MemoryViewerPanel::OnChangeToolsAddr) );
	Connect(sc_bytes->GetId(), wxEVT_COMMAND_TEXT_ENTER,       wxCommandEventHandler(MemoryViewerPanel::OnChangeToolsBytes) );
	Connect(sc_bytes->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler(MemoryViewerPanel::OnChangeToolsBytes) );

	Connect(b_prev->GetId(),  wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MemoryViewerPanel::Prev));
	Connect(b_next->GetId(),  wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MemoryViewerPanel::Next));
	Connect(b_fprev->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MemoryViewerPanel::fPrev));
	Connect(b_fnext->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MemoryViewerPanel::fNext));
	Connect(b_img->GetId(),   wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MemoryViewerPanel::OnShowImage));

	t_mem_addr ->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(MemoryViewerPanel::OnScrollMemory), NULL, this);
	t_mem_hex  ->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(MemoryViewerPanel::OnScrollMemory), NULL, this);
	t_mem_ascii->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(MemoryViewerPanel::OnScrollMemory), NULL, this);
	
	//Fill the wxTextCtrl's
	ShowMemory();
};

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

void MemoryViewerPanel::OnShowImage(wxCommandEvent& WXUNUSED(event))
{
	u32 addr = m_addr;
	int mode = cbox_img_mode->GetSelection();
	int sizex = sc_img_size_x->GetValue();
	int sizey = sc_img_size_y->GetValue();
	ShowImage(this, m_addr, mode, sizex, sizey, false);
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

void MemoryViewerPanel::ShowImage(wxWindow* parent, u32 addr, int mode, int width, int height, bool flipv)
{
	wxString title = wxString::Format("Raw Image @ 0x%x", addr);
	
	wxFrame* f_image_viewer = new wxFrame(parent, wxID_ANY,  title, wxDefaultPosition, wxDefaultSize,
		wxSYSTEM_MENU | wxMINIMIZE_BOX | wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN);
	f_image_viewer->SetBackgroundColour(wxColour(240,240,240)); //This fix the ugly background color under Windows
	f_image_viewer->SetAutoLayout(true);
	f_image_viewer->SetClientSize(wxSize(width, height));
	f_image_viewer->Show();
	wxClientDC dc_canvas(f_image_viewer);

	unsigned char* originalBuffer  = (unsigned char*)Memory.VirtualToRealAddr(addr);
	unsigned char* convertedBuffer = (unsigned char*)malloc(width * height * 3);
	switch(mode)
	{
	case(0): // RGB
		memcpy(convertedBuffer, originalBuffer, width * height * 3);
	break;

	case(1): // ARGB
		for (u32 y=0; y<height; y++){
			for (u32 i=0, j=0; j<width*4; i+=3, j+=4){
				convertedBuffer[i+0 + y*width*3] = originalBuffer[j+1 + y*width*4];
				convertedBuffer[i+1 + y*width*3] = originalBuffer[j+2 + y*width*4];
				convertedBuffer[i+2 + y*width*3] = originalBuffer[j+3 + y*width*4];
			}
		}
	break;

	case(2): // RGBA
		for (u32 y=0; y<height; y++){
			for (u32 i=0, j=0; j<width*4; i+=3, j+=4){
				convertedBuffer[i+0 + y*width*3] = originalBuffer[j+0 + y*width*4];
				convertedBuffer[i+1 + y*width*3] = originalBuffer[j+1 + y*width*4];
				convertedBuffer[i+2 + y*width*3] = originalBuffer[j+2 + y*width*4];
			}
		}
	break;

	case(3): // ABGR
		for (u32 y=0; y<height; y++){
			for (u32 i=0, j=0; j<width*4; i+=3, j+=4){
				convertedBuffer[i+0 + y*width*3] = originalBuffer[j+3 + y*width*4];
				convertedBuffer[i+1 + y*width*3] = originalBuffer[j+2 + y*width*4];
				convertedBuffer[i+2 + y*width*3] = originalBuffer[j+1 + y*width*4];
			}
		}
	break;
	}

	// Flip vertically
	if (flipv){
		for (u32 y=0; y<height/2; y++){
			for (u32 x=0; x<width*3; x++){
				const u8 t = convertedBuffer[x + y*width*3];
				convertedBuffer[x + y*width*3] = convertedBuffer[x + (height-y-1)*width*3];
				convertedBuffer[x + (height-y-1)*width*3] = t;
			}
		}
	}

	wxImage img(width, height, convertedBuffer);
	dc_canvas.DrawBitmap(img, 0, 0, false);
}

void MemoryViewerPanel::Next (wxCommandEvent& WXUNUSED(event)) { m_addr += m_colcount; ShowMemory(); }
void MemoryViewerPanel::Prev (wxCommandEvent& WXUNUSED(event)) { m_addr -= m_colcount; ShowMemory(); }
void MemoryViewerPanel::fNext(wxCommandEvent& WXUNUSED(event)) { m_addr += m_rowcount * m_colcount; ShowMemory(); }
void MemoryViewerPanel::fPrev(wxCommandEvent& WXUNUSED(event)) { m_addr -= m_rowcount * m_colcount; ShowMemory(); }
