#include "stdafx.h"
#include "RSXDebugger.h"
#include "Emu/Memory/Memory.h"
#include "Emu/GS/sysutil_video.h"

const char* s_rsx_flags [] = {
	"Alpha test",
	"Blend",
	"Cull face",
	"Depth bounds test",
	"Depth test",
	"Dither",
	"Line smooth",
	"Logic op",
	"Poly smooth",
	"Poly offset fill",
	"Poly offset line",
	"Poly offset point",
	"Stencil test",
};

RSXDebugger::RSXDebugger(wxWindow* parent) 
	: wxFrame(parent, wxID_ANY, "RSX Debugger", wxDefaultPosition, wxSize(700, 450))
	, m_item_count(23)
{
	exit = false;
	m_addr = 0x0;
	m_colcount = 16;

	this->SetBackgroundColour(wxColour(240,240,240)); //This fix the ugly background color under Windows
	wxBoxSizer& s_panel = *new wxBoxSizer(wxHORIZONTAL);

	//Tools
	wxBoxSizer& s_tools = *new wxBoxSizer(wxVERTICAL);

	//Controls: Memory Viewer Options
	wxStaticBoxSizer& s_controls = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Memory Viewer Options");

	wxStaticBoxSizer& s_controls_addr = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Address:");
	t_addr = new wxTextCtrl(this, wxID_ANY, "00000000", wxDefaultPosition, wxSize(60,-1));
	t_addr->SetMaxLength(8);
	s_controls_addr.Add(t_addr);

	wxStaticBoxSizer& s_controls_goto = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Go to:");
	wxButton* b_goto_get = new wxButton(this, wxID_ANY, "Get", wxDefaultPosition, wxSize(40,-1));
	wxButton* b_goto_put = new wxButton(this, wxID_ANY, "Put", wxDefaultPosition, wxSize(40,-1));
	s_controls_goto.Add(b_goto_get);
	s_controls_goto.Add(b_goto_put);

	//Controls: Breaks
	wxStaticBoxSizer& s_controls_breaks = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Break on:");
	wxButton* b_break_frame = new wxButton(this, wxID_ANY, "Frame",     wxDefaultPosition, wxSize(60,-1));
	wxButton* b_break_text  = new wxButton(this, wxID_ANY, "Texture",   wxDefaultPosition, wxSize(60,-1));
	wxButton* b_break_draw  = new wxButton(this, wxID_ANY, "Draw",      wxDefaultPosition, wxSize(60,-1));
	wxButton* b_break_prim  = new wxButton(this, wxID_ANY, "Primitive", wxDefaultPosition, wxSize(60,-1));
	wxButton* b_break_inst  = new wxButton(this, wxID_ANY, "Command",   wxDefaultPosition, wxSize(60,-1));
	s_controls_breaks.Add(b_break_frame);
	s_controls_breaks.Add(b_break_text);
	s_controls_breaks.Add(b_break_draw);
	s_controls_breaks.Add(b_break_prim);
	s_controls_breaks.Add(b_break_inst);

	// TODO: This feature is not yet implemented
	b_break_frame->Disable();
	b_break_text->Disable();
	b_break_draw->Disable();
	b_break_prim->Disable();
	b_break_inst->Disable();
	
	s_controls.Add(&s_controls_addr);
	s_controls.Add(&s_controls_goto);
	s_controls.Add(&s_controls_breaks);

	//Tabs
	wxNotebook* nb_rsx = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxSize(482,475));
	wxPanel* p_commands  = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_flags     = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_lightning = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_texture   = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_settings  = new wxPanel(nb_rsx, wxID_ANY);

	nb_rsx->AddPage(p_commands,  wxT("RSX Commands"));
	nb_rsx->AddPage(p_flags,     wxT("Flags"));
	nb_rsx->AddPage(p_lightning, wxT("Lightning"));
	nb_rsx->AddPage(p_texture,   wxT("Texture"));
	nb_rsx->AddPage(p_settings,  wxT("Settings"));

	//Tabs: Lists
	m_list_commands  = new wxListView(p_commands,  wxID_ANY, wxPoint(1,3), wxSize(470,444));
	m_list_flags     = new wxListView(p_flags,     wxID_ANY, wxPoint(1,3), wxSize(470,444));
	m_list_lightning = new wxListView(p_lightning, wxID_ANY, wxPoint(1,3), wxSize(470,444));
	m_list_texture   = new wxListView(p_texture,   wxID_ANY, wxPoint(1,3), wxSize(470,444));
	m_list_settings  = new wxListView(p_settings,  wxID_ANY, wxPoint(1,3), wxSize(470,444));
	
	//Tabs: List Style
	m_list_commands ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_flags    ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_lightning->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_texture  ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_settings ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	//Tabs: List Columns
	m_list_commands->InsertColumn(0, "Address", 0, 100);
	m_list_commands->InsertColumn(1, "Value", 0, 100);
	m_list_commands->InsertColumn(2, "Command", 0, 250);
	m_list_flags->InsertColumn(0, "Name", 0, 150);
	m_list_flags->InsertColumn(1, "Value", 0, 300);
	m_list_lightning->InsertColumn(0, "Name");
	m_list_lightning->InsertColumn(1, "Value");
	m_list_texture->InsertColumn(0, "Name");
	m_list_texture->InsertColumn(1, "Value");
	m_list_settings->InsertColumn(0, "Name");
	m_list_settings->InsertColumn(1, "Value");

	for(u32 i=0; i<m_item_count; i++)
	{
		m_list_commands->InsertItem(m_list_commands->GetItemCount(), wxEmptyString);
	}

	for(u32 i=0; i < sizeof(s_rsx_flags)/sizeof(s_rsx_flags[0]); i++)
	{
		m_list_flags->InsertItem(m_list_flags->GetItemCount(), s_rsx_flags[i]);
	}
	
	//Tools: Tools = Memory Viewer Options + Raw Image Preview Options + Buttons
	s_tools.AddSpacer(10);
	s_tools.Add(&s_controls);
	s_tools.AddSpacer(10);
	s_tools.Add(nb_rsx);
	s_tools.AddSpacer(10);

	//Buffers
	wxBoxSizer& s_buffers1 = *new wxBoxSizer(wxVERTICAL);
	wxBoxSizer& s_buffers2 = *new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer& s_buffers_colorA   = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Color Buffer A");
	wxStaticBoxSizer& s_buffers_colorB   = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Color Buffer B");
	wxStaticBoxSizer& s_buffers_colorC   = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Color Buffer C");
	wxStaticBoxSizer& s_buffers_colorD   = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Color Buffer D");
	wxStaticBoxSizer& s_buffers_depth   = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Depth Buffer");
	wxStaticBoxSizer& s_buffers_stencil = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Stencil Buffer");
	wxStaticBoxSizer& s_buffers_text    = *new wxStaticBoxSizer(wxHORIZONTAL, this, "Texture");

	//Buffers and textures
	CellVideoOutResolution res  = ResolutionTable[ResolutionIdToNum(Ini.GSResolution.GetValue())];
	m_buffers_width = res.width;
	m_buffers_height = res.height;
	m_panel_width = (m_buffers_width*108)/m_buffers_height;
	m_panel_height = 108;
	m_text_width = 108;
	m_text_height = 108;

	//Panels for displaying the buffers
	p_buffer_colorA  = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(m_panel_width, m_panel_height));
	p_buffer_colorB  = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(m_panel_width, m_panel_height));
	p_buffer_colorC  = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(m_panel_width, m_panel_height));
	p_buffer_colorD  = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(m_panel_width, m_panel_height));
	p_buffer_depth   = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(m_panel_width, m_panel_height));
	p_buffer_stencil = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(m_panel_width, m_panel_height));
	p_buffer_text    = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(m_text_width, m_text_height));
	s_buffers_colorA.Add(p_buffer_colorA); 
	s_buffers_colorB.Add(p_buffer_colorB); 
	s_buffers_colorC.Add(p_buffer_colorC); 
	s_buffers_colorD.Add(p_buffer_colorD); 
	s_buffers_depth.Add(p_buffer_depth);
	s_buffers_stencil.Add(p_buffer_stencil);
	s_buffers_text.Add(p_buffer_text);
	
	//Merge and display everything
	s_buffers1.AddSpacer(10);
	s_buffers1.Add(&s_buffers_colorA);
	s_buffers1.AddSpacer(10);
	s_buffers1.Add(&s_buffers_colorC);
	s_buffers1.AddSpacer(10);
	s_buffers1.Add(&s_buffers_depth);
	s_buffers1.AddSpacer(10);
	s_buffers1.Add(&s_buffers_text);
	s_buffers1.AddSpacer(10);

	s_buffers2.AddSpacer(10);
	s_buffers2.Add(&s_buffers_colorB);
	s_buffers2.AddSpacer(10);
	s_buffers2.Add(&s_buffers_colorD);
	s_buffers2.AddSpacer(10);
	s_buffers2.Add(&s_buffers_stencil);
	s_buffers2.AddSpacer(10);

	s_panel.AddSpacer(10);
	s_panel.Add(&s_tools);
	s_panel.AddSpacer(10);
	s_panel.Add(&s_buffers1);
	s_panel.AddSpacer(10);
	s_panel.Add(&s_buffers2);
	s_panel.AddSpacer(10);
	SetSizerAndFit(&s_panel);

	//Events
	Connect(wxID_ANY, wxEVT_KEY_DOWN, wxKeyEventHandler(RSXDebugger::OnKeyDown), NULL, this);
	Connect(t_addr->GetId(),   wxEVT_COMMAND_TEXT_ENTER,       wxCommandEventHandler(RSXDebugger::OnChangeToolsAddr));

	Connect(b_goto_get->GetId(),  wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(RSXDebugger::GoToGet));
	Connect(b_goto_put->GetId(),  wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(RSXDebugger::GoToPut));

	m_list_commands->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(RSXDebugger::OnScrollMemory), NULL, this);
	m_list_flags->Connect(wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler(RSXDebugger::ModifyFlags), NULL, this);
	
	//Fill the frame
	UpdateInformation();
};

void RSXDebugger::OnKeyDown(wxKeyEvent& event)
{
	switch(event.GetKeyCode())
	{
	case WXK_F5: UpdateInformation(); break;
	}
}

void RSXDebugger::OnChangeToolsAddr(wxCommandEvent& event)
{
	t_addr->GetValue().ToULong((unsigned long *)&m_addr, 16);
	t_addr->SetValue(wxString::Format("%08x", m_addr));
	UpdateInformation();
	event.Skip();
}

void RSXDebugger::OnScrollMemory(wxMouseEvent& event)
{
	m_addr -= (event.ControlDown() ? m_item_count : 1) * 4 * (event.GetWheelRotation() / event.GetWheelDelta());
	t_addr->SetValue(wxString::Format("%08x", m_addr));
	UpdateInformation();
	event.Skip();
}

void RSXDebugger::GoToGet(wxCommandEvent& event)
{
	if (!RSXReady()) return;
	CellGcmControl* ctrl = (CellGcmControl*)&Memory[Emu.GetGSManager().GetRender().m_ctrlAddress];
	m_addr = Emu.GetGSManager().GetRender().m_ioAddress + re(ctrl->get);
	t_addr->SetValue(wxString::Format("%08x", m_addr));
	UpdateInformation();
	event.Skip();
}

void RSXDebugger::GoToPut(wxCommandEvent& event)
{
	if (!RSXReady()) return;
	CellGcmControl* ctrl = (CellGcmControl*)&Memory[Emu.GetGSManager().GetRender().m_ctrlAddress];
	m_addr = Emu.GetGSManager().GetRender().m_ioAddress + re(ctrl->put);
	t_addr->SetValue(wxString::Format("%08x", m_addr));
	UpdateInformation();
	event.Skip();
}

void RSXDebugger::UpdateInformation()
{
	ShowMemory();
	ShowBuffers();
	ShowFlags();
	this->SetFocus();
}

void RSXDebugger::ShowMemory()
{
	for(u32 i=0; i<m_item_count; i++)
	{
		u32 addr =  m_addr + 4*i;
		m_list_commands->SetItem(i, 0, wxString::Format("%08x", addr));
	
		if (Memory.IsGoodAddr(addr))
		{
			u32 cmd = Memory.Read32(addr);
			m_list_commands->SetItem(i, 1, wxString::Format("%08x", cmd));
		}
		else
		{
			m_list_commands->SetItem(i, 1, "????????");
		}
	}
}

void RSXDebugger::ShowBuffers()
{
	// TODO: This is a *very* ugly way of checking if m_render was initialized. It throws an error while debugging with VS
	const GSRender& render = Emu.GetGSManager().GetRender();
	if (!&render)
		return;

	// TODO: Currently it only supports color buffers
	for (u32 bufferId=0; bufferId < render.m_gcm_buffers_count; bufferId++)
	{
		gcmBuffer* buffers = (gcmBuffer*)Memory.GetMemFromAddr(render.m_gcm_buffers_addr);
		u32 RSXbuffer_addr = render.m_local_mem_addr + re(buffers[bufferId].offset);
		unsigned char* RSXbuffer = (unsigned char*)Memory.VirtualToRealAddr(RSXbuffer_addr);
		
		u32 width  = re(buffers[bufferId].width);
		u32 height = re(buffers[bufferId].height);
		unsigned char* buffer = (unsigned char*)malloc(width * height * 3);

		//ABGR to RGB and flip vertically
		for (u32 y=0; y<height; y++){
			for (u32 i=0, j=0; j<width*4; i+=3, j+=4)
			{
				buffer[i+0 + y*width*3] = RSXbuffer[j+3 + (height-y-1)*width*4];
				buffer[i+1 + y*width*3] = RSXbuffer[j+2 + (height-y-1)*width*4];
				buffer[i+2 + y*width*3] = RSXbuffer[j+1 + (height-y-1)*width*4];
			}
		}

		// TODO: Is there any better way to clasify the color buffers? How can we include the depth and stencil buffers?
		wxPanel* pnl;
		switch(bufferId)
		{
		case 0:	 pnl = p_buffer_colorA;  break;
		case 1:	 pnl = p_buffer_colorB;  break;
		case 2:	 pnl = p_buffer_colorC;  break;
		default: pnl = p_buffer_colorD;  break;
		}

		wxImage img(m_buffers_width, m_buffers_height, buffer);
		wxClientDC dc_canvas(pnl);
		dc_canvas.DrawBitmap(img.Scale(m_panel_width, m_panel_height), 0, 0, false);
	}
}

void RSXDebugger::ShowFlags()
{
	if (!RSXReady())
	{
		for (u32 i=0; i<m_list_flags->GetItemCount(); i++)
		{
			m_list_flags->SetItem(i, 1, "N/A");
		}
		return;
	}
	const GSRender& render = Emu.GetGSManager().GetRender();
	int i=0;
	m_list_flags->SetItem(i++, 1, render.m_set_alpha_test        ? "Enabled" : "Disabled");
	m_list_flags->SetItem(i++, 1, render.m_set_blend             ? "Enabled" : "Disabled");
	m_list_flags->SetItem(i++, 1, render.m_set_cull_face_enable  ? "Enabled" : "Disabled");
	m_list_flags->SetItem(i++, 1, render.m_set_depth_bounds_test ? "Enabled" : "Disabled");
	m_list_flags->SetItem(i++, 1, render.m_depth_test_enable     ? "Enabled" : "Disabled");
	m_list_flags->SetItem(i++, 1, render.m_set_dither            ? "Enabled" : "Disabled");
	m_list_flags->SetItem(i++, 1, render.m_set_line_smooth       ? "Enabled" : "Disabled");
	m_list_flags->SetItem(i++, 1, render.m_set_logic_op          ? "Enabled" : "Disabled");
	m_list_flags->SetItem(i++, 1, render.m_set_poly_smooth       ? "Enabled" : "Disabled");
	m_list_flags->SetItem(i++, 1, render.m_set_poly_offset_fill  ? "Enabled" : "Disabled");
	m_list_flags->SetItem(i++, 1, render.m_set_poly_offset_line  ? "Enabled" : "Disabled");
	m_list_flags->SetItem(i++, 1, render.m_set_poly_offset_point ? "Enabled" : "Disabled");
	m_list_flags->SetItem(i++, 1, render.m_set_stencil_test      ? "Enabled" : "Disabled");
}

void RSXDebugger::ModifyFlags(wxListEvent& event)
{
	if (!RSXReady()) return;
	GSRender& render = Emu.GetGSManager().GetRender();
	switch(event.m_itemIndex)
	{
	case 0:  render.m_set_alpha_test        ^= true; break;
	case 1:  render.m_set_blend             ^= true; break;
	case 2:  render.m_set_cull_face_enable  ^= true; break;
	case 3:  render.m_set_depth_bounds_test ^= true; break;
	case 4:  render.m_depth_test_enable     ^= true; break;
	case 5:  render.m_set_dither            ^= true; break;
	case 6:  render.m_set_line_smooth       ^= true; break;
	case 7:  render.m_set_logic_op          ^= true; break;
	case 8:  render.m_set_poly_smooth       ^= true; break;
	case 9:  render.m_set_poly_offset_fill  ^= true; break;
	case 10: render.m_set_poly_offset_line  ^= true; break;
	case 11: render.m_set_poly_offset_point ^= true; break;
	case 12: render.m_set_stencil_test      ^= true; break;
	}
	UpdateInformation();
}

bool RSXDebugger::RSXReady()
{
	// TODO: This is a *very* ugly way of checking if m_render was initialized. It throws an error while debugging in VS
	const GSRender& render = Emu.GetGSManager().GetRender();
	if (!&render)
	{
		return false;
	}
	return true;
}

