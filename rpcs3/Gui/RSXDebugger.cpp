#include "stdafx_gui.h"
#include "rpcs3/Ini.h"
#include "Utilities/rPlatform.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "RSXDebugger.h"
#include "Emu/RSX/sysutil_video.h"
#include "Emu/RSX/GSManager.h"
#include "Emu/RSX/GSRender.h"
//#include "Emu/RSX/GCM.h"

#include "MemoryViewer.h"

#include <wx/notebook.h>

// TODO: Clear the object when restarting the emulator
std::vector<RSXDebuggerProgram> m_debug_programs;

enum GCMEnumTypes
{
	CELL_GCM_ENUM,
	CELL_GCM_PRIMITIVE_ENUM,
};

RSXDebugger::RSXDebugger(wxWindow* parent) 
	: wxFrame(parent, wxID_ANY, "RSX Debugger", wxDefaultPosition, wxSize(700, 450))
	, m_item_count(37)
	, m_addr(0x0)
	, m_cur_texture(0)
	, exit(false)
{
	this->SetBackgroundColour(wxColour(240,240,240)); //This fix the ugly background color under Windows
	wxBoxSizer* s_panel = new wxBoxSizer(wxHORIZONTAL);

	//Tools
	wxBoxSizer* s_tools = new wxBoxSizer(wxVERTICAL);

	// Controls
	wxStaticBoxSizer* s_controls = new wxStaticBoxSizer(wxHORIZONTAL, this, "RSX Debugger Controls");

	// Controls: Address
	wxStaticBoxSizer* s_controls_addr = new wxStaticBoxSizer(wxHORIZONTAL, this, "Address:");
	t_addr = new wxTextCtrl(this, wxID_ANY, "00000000", wxDefaultPosition, wxSize(60,-1));
	t_addr->SetMaxLength(8);
	s_controls_addr->Add(t_addr);

	// Controls: Go to
	wxStaticBoxSizer* s_controls_goto = new wxStaticBoxSizer(wxHORIZONTAL, this, "Go to:");
	wxButton* b_goto_get = new wxButton(this, wxID_ANY, "Get", wxDefaultPosition, wxSize(40,-1));
	wxButton* b_goto_put = new wxButton(this, wxID_ANY, "Put", wxDefaultPosition, wxSize(40,-1));
	s_controls_goto->Add(b_goto_get);
	s_controls_goto->Add(b_goto_put);

	// Controls: Breaks
	wxStaticBoxSizer* s_controls_breaks = new wxStaticBoxSizer(wxHORIZONTAL, this, "Break on:");
	wxButton* b_break_frame = new wxButton(this, wxID_ANY, "Frame",     wxDefaultPosition, wxSize(60,-1));
	wxButton* b_break_text  = new wxButton(this, wxID_ANY, "Texture",   wxDefaultPosition, wxSize(60,-1));
	wxButton* b_break_draw  = new wxButton(this, wxID_ANY, "Draw",      wxDefaultPosition, wxSize(60,-1));
	wxButton* b_break_prim  = new wxButton(this, wxID_ANY, "Primitive", wxDefaultPosition, wxSize(60,-1));
	wxButton* b_break_inst  = new wxButton(this, wxID_ANY, "Command",   wxDefaultPosition, wxSize(60,-1));
	s_controls_breaks->Add(b_break_frame);
	s_controls_breaks->Add(b_break_text);
	s_controls_breaks->Add(b_break_draw);
	s_controls_breaks->Add(b_break_prim);
	s_controls_breaks->Add(b_break_inst);

	// TODO: This feature is not yet implemented
	b_break_frame->Disable();
	b_break_text->Disable();
	b_break_draw->Disable();
	b_break_prim->Disable();
	b_break_inst->Disable();
	
	s_controls->Add(s_controls_addr);
	s_controls->Add(s_controls_goto);
	s_controls->Add(s_controls_breaks);

	//Tabs
	wxNotebook* nb_rsx = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxSize(732, 732));
	wxPanel* p_commands  = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_flags     = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_programs  = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_lightning = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_texture   = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_settings  = new wxPanel(nb_rsx, wxID_ANY);

	nb_rsx->AddPage(p_commands,  wxT("RSX Commands"));
	nb_rsx->AddPage(p_flags,     wxT("Flags"));
	nb_rsx->AddPage(p_programs,  wxT("Programs"));
	nb_rsx->AddPage(p_lightning, wxT("Lightning"));
	nb_rsx->AddPage(p_texture,   wxT("Texture"));
	nb_rsx->AddPage(p_settings,  wxT("Settings"));

	//Tabs: Lists
	m_list_commands  = new wxListView(p_commands,  wxID_ANY, wxPoint(1,3), wxSize(720, 720));
	m_list_flags     = new wxListView(p_flags,     wxID_ANY, wxPoint(1,3), wxSize(720, 720));
	m_list_programs  = new wxListView(p_programs,  wxID_ANY, wxPoint(1,3), wxSize(720, 720));
	m_list_lightning = new wxListView(p_lightning, wxID_ANY, wxPoint(1,3), wxSize(720, 720));
	m_list_texture   = new wxListView(p_texture,   wxID_ANY, wxPoint(1,3), wxSize(720, 720));
	m_list_settings  = new wxListView(p_settings,  wxID_ANY, wxPoint(1,3), wxSize(720, 720));
	
	//Tabs: List Style
	m_list_commands ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_flags    ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_programs ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_lightning->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_texture  ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_settings ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	//Tabs: List Columns
	m_list_commands->InsertColumn(0, "Address", 0, 80);
	m_list_commands->InsertColumn(1, "Value", 0, 80);
	m_list_commands->InsertColumn(2, "Command", 0, 500);
	m_list_commands->InsertColumn(3, "Count", 0, 40);
	m_list_flags->InsertColumn(0, "Name", 0, 170);
	m_list_flags->InsertColumn(1, "Value", 0, 270);
	m_list_programs->InsertColumn(0, "ID", 0, 70);
	m_list_programs->InsertColumn(1, "VP ID", 0, 70);
	m_list_programs->InsertColumn(2, "FP ID", 0, 70);
	m_list_programs->InsertColumn(3, "VP Length", 0, 110);
	m_list_programs->InsertColumn(4, "FP Length", 0, 110);
	m_list_lightning->InsertColumn(0, "Name", 0, 170);
	m_list_lightning->InsertColumn(1, "Value", 0, 270);

	m_list_texture->InsertColumn(0, "Index");
	m_list_texture->InsertColumn(1, "Address");
	m_list_texture->InsertColumn(2, "Cubemap");
	m_list_texture->InsertColumn(3, "Dimension");
	m_list_texture->InsertColumn(4, "Enabled");
	m_list_texture->InsertColumn(5, "Format");
	m_list_texture->InsertColumn(6, "Mipmap");
	m_list_texture->InsertColumn(7, "Pitch");
	m_list_texture->InsertColumn(8, "Size");

	m_list_settings->InsertColumn(0, "Name", 0, 170);
	m_list_settings->InsertColumn(1, "Value", 0, 270);

	// Fill list
	for(u32 i=0; i<m_item_count; i++)
	{
		m_list_commands->InsertItem(m_list_commands->GetItemCount(), wxEmptyString);
	}

	//Tools: Tools = Controls + Notebook Tabs
	s_tools->AddSpacer(10);
	s_tools->Add(s_controls);
	s_tools->AddSpacer(10);
	s_tools->Add(nb_rsx);
	s_tools->AddSpacer(10);

	//Buffers
	wxBoxSizer* s_buffers1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_buffers2 = new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer* s_buffers_colorA  = new wxStaticBoxSizer(wxHORIZONTAL, this, "Color Buffer A");
	wxStaticBoxSizer* s_buffers_colorB  = new wxStaticBoxSizer(wxHORIZONTAL, this, "Color Buffer B");
	wxStaticBoxSizer* s_buffers_colorC  = new wxStaticBoxSizer(wxHORIZONTAL, this, "Color Buffer C");
	wxStaticBoxSizer* s_buffers_colorD  = new wxStaticBoxSizer(wxHORIZONTAL, this, "Color Buffer D");
	wxStaticBoxSizer* s_buffers_depth   = new wxStaticBoxSizer(wxHORIZONTAL, this, "Depth Buffer");
	wxStaticBoxSizer* s_buffers_stencil = new wxStaticBoxSizer(wxHORIZONTAL, this, "Stencil Buffer");
	wxStaticBoxSizer* s_buffers_text    = new wxStaticBoxSizer(wxHORIZONTAL, this, "Texture");

	//Buffers and textures
	CellVideoOutResolution res  = ResolutionTable[ResolutionIdToNum(Ini.GSResolution.GetValue())];
	m_panel_width = (res.width*108)/res.height;
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
	p_buffer_tex     = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(m_text_width, m_text_height));
	s_buffers_colorA->Add(p_buffer_colorA); 
	s_buffers_colorB->Add(p_buffer_colorB); 
	s_buffers_colorC->Add(p_buffer_colorC); 
	s_buffers_colorD->Add(p_buffer_colorD); 
	s_buffers_depth->Add(p_buffer_depth);
	s_buffers_stencil->Add(p_buffer_stencil);
	s_buffers_text->Add(p_buffer_tex);
	
	//Merge and display everything
	s_buffers1->AddSpacer(10);
	s_buffers1->Add(s_buffers_colorA);
	s_buffers1->AddSpacer(10);
	s_buffers1->Add(s_buffers_colorC);
	s_buffers1->AddSpacer(10);
	s_buffers1->Add(s_buffers_depth);
	s_buffers1->AddSpacer(10);
	s_buffers1->Add(s_buffers_text);
	s_buffers1->AddSpacer(10);

	s_buffers2->AddSpacer(10);
	s_buffers2->Add(s_buffers_colorB);
	s_buffers2->AddSpacer(10);
	s_buffers2->Add(s_buffers_colorD);
	s_buffers2->AddSpacer(10);
	s_buffers2->Add(s_buffers_stencil);
	s_buffers2->AddSpacer(10);

	s_panel->AddSpacer(10);
	s_panel->Add(s_tools);
	s_panel->AddSpacer(10);
	s_panel->Add(s_buffers1);
	s_panel->AddSpacer(10);
	s_panel->Add(s_buffers2);
	s_panel->AddSpacer(10);
	SetSizerAndFit(s_panel);

	//Events
	Bind(wxEVT_KEY_DOWN, &RSXDebugger::OnKeyDown, this);
	t_addr->Bind(wxEVT_TEXT_ENTER, &RSXDebugger::OnChangeToolsAddr, this);

	b_goto_get->Bind(wxEVT_BUTTON, &RSXDebugger::GoToGet, this);
	b_goto_put->Bind(wxEVT_BUTTON, &RSXDebugger::GoToPut, this);

	p_buffer_colorA->Bind(wxEVT_LEFT_DOWN, &RSXDebugger::OnClickBuffer, this);
	p_buffer_colorB->Bind(wxEVT_LEFT_DOWN, &RSXDebugger::OnClickBuffer, this);
	p_buffer_colorC->Bind(wxEVT_LEFT_DOWN, &RSXDebugger::OnClickBuffer, this);
	p_buffer_colorD->Bind(wxEVT_LEFT_DOWN, &RSXDebugger::OnClickBuffer, this);
	//p_buffer_depth->Bind(wxEVT_BUTTON, &RSXDebugger::OnClickBuffer, this);
	//p_buffer_stencil->Bind(wxEVT_BUTTON, &RSXDebugger::OnClickBuffer, this);
	p_buffer_tex->Bind(wxEVT_LEFT_DOWN, &RSXDebugger::OnClickBuffer, this);

	m_list_commands->Bind(wxEVT_MOUSEWHEEL, &RSXDebugger::OnScrollMemory, this);
	m_list_flags->Bind(wxEVT_LIST_ITEM_ACTIVATED, &RSXDebugger::SetFlags, this);
	m_list_programs->Bind(wxEVT_LIST_ITEM_ACTIVATED, &RSXDebugger::SetPrograms, this);

	m_list_texture->Bind(wxEVT_LIST_ITEM_ACTIVATED, &RSXDebugger::OnSelectTexture, this);
	
	//Fill the frame
	UpdateInformation();
};

void RSXDebugger::OnKeyDown(wxKeyEvent& event)
{
	if(wxGetActiveWindow() == wxGetTopLevelParent(this))
	{
		switch(event.GetKeyCode())
		{
		case WXK_F5: UpdateInformation(); return;
		}
	}

	event.Skip();
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
	if(vm::check_addr(m_addr))
	{
		int items = event.ControlDown() ? m_item_count : 1;

		for(int i=0; i<items; ++i)
		{
			u32 offset;
			if(vm::check_addr(m_addr))
			{
				u32 cmd = vm::read32(m_addr);
				u32 count = (cmd & (CELL_GCM_METHOD_FLAG_JUMP | CELL_GCM_METHOD_FLAG_CALL))
					|| cmd == CELL_GCM_METHOD_FLAG_RETURN ? 0 : (cmd >> 18) & 0x7ff;

				offset = 1 + count;
			}
			else
			{
				offset = 1;
			}

			m_addr -= 4 * offset * (event.GetWheelRotation() / event.GetWheelDelta());
		}
	}
	else
	{
		m_addr -= (event.ControlDown() ? m_item_count : 1) * 4 * (event.GetWheelRotation() / event.GetWheelDelta());
	}

	UpdateInformation();
	event.Skip();
}

void RSXDebugger::OnClickBuffer(wxMouseEvent& event)
{
	if (!RSXReady()) return;
	const GSRender& render = Emu.GetGSManager().GetRender();
	const auto buffers = vm::ptr<CellGcmDisplayInfo>::make(render.m_gcm_buffers_addr);

	if(!buffers)
		return;

	// TODO: Is there any better way to choose the color buffers
#define SHOW_BUFFER(id) \
	{ \
		u32 addr = render.m_local_mem_addr + buffers[id].offset; \
		if (vm::check_addr(addr) && buffers[id].width && buffers[id].height) \
			MemoryViewerPanel::ShowImage(this, addr, 3, buffers[id].width, buffers[id].height, true); \
		return; \
	} \

	if (event.GetId() == p_buffer_colorA->GetId()) SHOW_BUFFER(0);
	if (event.GetId() == p_buffer_colorB->GetId()) SHOW_BUFFER(1);
	if (event.GetId() == p_buffer_colorC->GetId()) SHOW_BUFFER(2);
	if (event.GetId() == p_buffer_colorD->GetId()) SHOW_BUFFER(3);
	if (event.GetId() == p_buffer_tex->GetId())
	{
		u8 location = render.m_textures[m_cur_texture].GetLocation();
		if(location <= 1 && vm::check_addr(GetAddress(render.m_textures[m_cur_texture].GetOffset(), location))
			&& render.m_textures[m_cur_texture].GetWidth() && render.m_textures[m_cur_texture].GetHeight())
			MemoryViewerPanel::ShowImage(this,
				GetAddress(render.m_textures[m_cur_texture].GetOffset(), location), 1,
				render.m_textures[m_cur_texture].GetWidth(),
				render.m_textures[m_cur_texture].GetHeight(), false);
	}

#undef SHOW_BUFFER
}

void RSXDebugger::GoToGet(wxCommandEvent& event)
{
	if (!RSXReady()) return;
	auto ctrl = vm::get_ptr<CellGcmControl>(Emu.GetGSManager().GetRender().m_ctrlAddress);
	u32 realAddr;
	if (Memory.RSXIOMem.getRealAddr(ctrl->get.load(), realAddr)) {
		m_addr = realAddr;
		t_addr->SetValue(wxString::Format("%08x", m_addr));
		UpdateInformation();
		event.Skip();
	}
	// TODO: We should probably throw something? 
}

void RSXDebugger::GoToPut(wxCommandEvent& event)
{
	if (!RSXReady()) return;
	auto ctrl = vm::get_ptr<CellGcmControl>(Emu.GetGSManager().GetRender().m_ctrlAddress);
	u32 realAddr;
	if (Memory.RSXIOMem.getRealAddr(ctrl->put.load(), realAddr)) {
		m_addr = realAddr;
		t_addr->SetValue(wxString::Format("%08x", m_addr));
		UpdateInformation();
		event.Skip();
	}
	// TODO: We should probably throw something? 
}

void RSXDebugger::UpdateInformation()
{
	t_addr->SetValue(wxString::Format("%08x", m_addr));
	GetMemory();
	GetBuffers();
	GetFlags();
	GetPrograms();
	GetLightning();
	GetTexture();
	GetSettings();
}

void RSXDebugger::GetMemory()
{
	// Clean commands column
	for(u32 i=0; i<m_item_count; i++)
		m_list_commands->SetItem(i, 2, wxEmptyString);

	bool isReady = RSXReady();

	// Write information
	for(u32 i=0, addr = m_addr; i<m_item_count; i++, addr += 4)
	{
		m_list_commands->SetItem(i, 0, wxString::Format("%08x", addr));
	
		if (isReady && vm::check_addr(addr))
		{
			u32 cmd = vm::read32(addr);
			u32 count = (cmd >> 18) & 0x7ff;
			m_list_commands->SetItem(i, 1, wxString::Format("%08x", cmd));
			m_list_commands->SetItem(i, 3, wxString::Format("%d", count));
			m_list_commands->SetItem(i, 2, DisAsmCommand(cmd, count, addr, 0));

			if(!(cmd & (CELL_GCM_METHOD_FLAG_JUMP | CELL_GCM_METHOD_FLAG_CALL)) && cmd != CELL_GCM_METHOD_FLAG_RETURN)
			{
				addr += 4 * count;
			}
		}
		else
		{
			m_list_commands->SetItem(i, 1, "????????");
		}
	}
}

void RSXDebugger::GetBuffers()
{
	if (!RSXReady()) return;
	const GSRender& render = Emu.GetGSManager().GetRender();

	// Draw Buffers
	// TODO: Currently it only supports color buffers
	for (u32 bufferId=0; bufferId < render.m_gcm_buffers_count; bufferId++)
	{
		if(!vm::check_addr(render.m_gcm_buffers_addr))
			continue;

		auto buffers = vm::get_ptr<CellGcmDisplayInfo>(render.m_gcm_buffers_addr);
		u32 RSXbuffer_addr = render.m_local_mem_addr + buffers[bufferId].offset;

		if(!vm::check_addr(RSXbuffer_addr))
			continue;

		auto RSXbuffer = vm::get_ptr<unsigned char>(RSXbuffer_addr);
		
		u32 width  = buffers[bufferId].width;
		u32 height = buffers[bufferId].height;
		unsigned char* buffer = (unsigned char*)malloc(width * height * 3);

		// ABGR to RGB and flip vertically
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

		wxImage img(width, height, buffer);
		wxClientDC dc_canvas(pnl);
		
		if (img.IsOk())
			dc_canvas.DrawBitmap(img.Scale(m_panel_width, m_panel_height), 0, 0, false);
	}

	// Draw Texture
	if(!render.m_textures[m_cur_texture].IsEnabled())
		return;

	u32 offset = render.m_textures[m_cur_texture].GetOffset();

	if(!offset)
		return;

	u8 location = render.m_textures[m_cur_texture].GetLocation();

	if(location > 1)
		return;

	u32 TexBuffer_addr = GetAddress(offset, location);

	if(!vm::check_addr(TexBuffer_addr))
		return;

	unsigned char* TexBuffer = vm::get_ptr<unsigned char>(TexBuffer_addr);

	u32 width  = render.m_textures[m_cur_texture].GetWidth();
	u32 height = render.m_textures[m_cur_texture].GetHeight();
	unsigned char* buffer = (unsigned char*)malloc(width * height * 3);
	memcpy(buffer, TexBuffer, width * height * 3);

	wxImage img(width, height, buffer);
	wxClientDC dc_canvas(p_buffer_tex);
	dc_canvas.DrawBitmap(img.Scale(m_text_width, m_text_height), 0, 0, false);
}

void RSXDebugger::GetFlags()
{
	if (!RSXReady()) return;
	const GSRender& render = Emu.GetGSManager().GetRender();
	m_list_flags->DeleteAllItems();
	int i=0;

#define LIST_FLAGS_ADD(name, value) \
	m_list_flags->InsertItem(i, name); m_list_flags->SetItem(i, 1, value ? "Enabled" : "Disabled"); i++;

	LIST_FLAGS_ADD("Alpha test",         render.m_set_alpha_test);
	LIST_FLAGS_ADD("Blend",              render.m_set_blend);
	LIST_FLAGS_ADD("Scissor",            render.m_set_scissor_horizontal && render.m_set_scissor_vertical);
	LIST_FLAGS_ADD("Cull face",          render.m_set_cull_face);
	LIST_FLAGS_ADD("Depth bounds test",  render.m_set_depth_bounds_test);
	LIST_FLAGS_ADD("Depth test",         render.m_set_depth_test);
	LIST_FLAGS_ADD("Dither",             render.m_set_dither);
	LIST_FLAGS_ADD("Line smooth",        render.m_set_line_smooth);
	LIST_FLAGS_ADD("Logic op",           render.m_set_logic_op);
	LIST_FLAGS_ADD("Poly smooth",        render.m_set_poly_smooth);
	LIST_FLAGS_ADD("Poly offset fill",   render.m_set_poly_offset_fill);
	LIST_FLAGS_ADD("Poly offset line",   render.m_set_poly_offset_line);
	LIST_FLAGS_ADD("Poly offset point",  render.m_set_poly_offset_point);
	LIST_FLAGS_ADD("Stencil test",       render.m_set_stencil_test);
	LIST_FLAGS_ADD("Primitive restart",  render.m_set_restart_index);
	LIST_FLAGS_ADD("Two sided lighting", render.m_set_two_side_light_enable);
	LIST_FLAGS_ADD("Point Sprite",	     render.m_set_point_sprite_control);
	LIST_FLAGS_ADD("Lighting ",	         render.m_set_specular);

#undef LIST_FLAGS_ADD
}

void RSXDebugger::GetPrograms()
{
	if (!RSXReady()) return;
	m_list_programs->DeleteAllItems();

	for (auto& program : m_debug_programs)
	{
		const int i = m_list_programs->InsertItem(m_list_programs->GetItemCount(), wxString::Format("%u", program.id));
		m_list_programs->SetItem(i, 1, wxString::Format("%u", program.vp_id));
		m_list_programs->SetItem(i, 2, wxString::Format("%u", program.fp_id));
		m_list_programs->SetItem(i, 3, wxString::Format("%llu", (u64)program.vp_shader.length()));
		m_list_programs->SetItem(i, 4, wxString::Format("%llu", (u64)program.fp_shader.length()));
	}
}

void RSXDebugger::GetLightning()
{
	if (!RSXReady()) return;
	const GSRender& render = Emu.GetGSManager().GetRender();
	m_list_lightning->DeleteAllItems();
	int i=0;

#define LIST_LIGHTNING_ADD(name, value) \
	m_list_lightning->InsertItem(i, name); m_list_lightning->SetItem(i, 1, value); i++;

	LIST_LIGHTNING_ADD("Shade model", (render.m_shade_mode == 0x1D00) ? "Flat" : "Smooth");

#undef LIST_LIGHTNING_ADD
}

void RSXDebugger::GetTexture()
{
	if (!RSXReady()) return;
	const GSRender& render = Emu.GetGSManager().GetRender();
	m_list_texture->DeleteAllItems();

	for(uint i=0; i<RSXThread::m_textures_count; ++i)
	{
		if(render.m_textures[i].IsEnabled())
		{
			m_list_texture->InsertItem(i, wxString::Format("%d", i));
			u8 location = render.m_textures[i].GetLocation();
			if(location > 1)
			{
				m_list_texture->SetItem(i, 1,
					wxString::Format("Bad address (offset=0x%x, location=%d)", render.m_textures[i].GetOffset(), location));
			}
			else
			{
				m_list_texture->SetItem(i, 1, wxString::Format("0x%x", GetAddress(render.m_textures[i].GetOffset(), location)));
			}

			m_list_texture->SetItem(i, 2, render.m_textures[i].isCubemap() ? "True" : "False");
			m_list_texture->SetItem(i, 3, wxString::Format("%dD", render.m_textures[i].GetDimension()));
			m_list_texture->SetItem(i, 4, render.m_textures[i].IsEnabled() ? "True" : "False");
			m_list_texture->SetItem(i, 5, wxString::Format("0x%x", render.m_textures[i].GetFormat()));
			m_list_texture->SetItem(i, 6, wxString::Format("0x%x", render.m_textures[i].GetMipmap()));
			m_list_texture->SetItem(i, 7, wxString::Format("0x%x", render.m_textures[i].m_pitch));
			m_list_texture->SetItem(i, 8, wxString::Format("%dx%d",
				render.m_textures[i].GetWidth(),
				render.m_textures[i].GetHeight()));

			m_list_texture->SetItemBackgroundColour(i, wxColour(m_cur_texture == i ? "Wheat" : "White"));
		}
	}
}

void RSXDebugger::GetSettings()
{
	if (!RSXReady()) return;
	const GSRender& render = Emu.GetGSManager().GetRender();
	m_list_settings->DeleteAllItems();
	int i=0;

#define LIST_SETTINGS_ADD(name, value) \
	m_list_settings->InsertItem(i, name); m_list_settings->SetItem(i, 1, value); i++;

	LIST_SETTINGS_ADD("Alpha func", !(render.m_set_alpha_func) ? "(none)" : wxString::Format("0x%x (%s)",
		render.m_alpha_func,
		ParseGCMEnum(render.m_alpha_func, CELL_GCM_ENUM)));
	LIST_SETTINGS_ADD("Blend color", !(render.m_set_blend_color) ? "(none)" : wxString::Format("R:%d, G:%d, B:%d, A:%d",
		render.m_blend_color_r,
		render.m_blend_color_g,
		render.m_blend_color_b,
		render.m_blend_color_a));
	LIST_SETTINGS_ADD("Clipping", wxString::Format("Min:%f, Max:%f", render.m_clip_min, render.m_clip_max));
	LIST_SETTINGS_ADD("Color mask", !(render.m_set_color_mask) ? "(none)" : wxString::Format("R:%d, G:%d, B:%d, A:%d",
		render.m_color_mask_r,
		render.m_color_mask_g,
		render.m_color_mask_b,
		render.m_color_mask_a));
	LIST_SETTINGS_ADD("Context DMA Color A", wxString::Format("0x%x", render.m_context_dma_color_a));
	LIST_SETTINGS_ADD("Context DMA Color B", wxString::Format("0x%x", render.m_context_dma_color_b));
	LIST_SETTINGS_ADD("Context DMA Color C", wxString::Format("0x%x", render.m_context_dma_color_c));
	LIST_SETTINGS_ADD("Context DMA Color D", wxString::Format("0x%x", render.m_context_dma_color_d));
	LIST_SETTINGS_ADD("Context DMA Zeta", wxString::Format("0x%x", render.m_context_dma_z));
	LIST_SETTINGS_ADD("Depth bounds", wxString::Format("Min:%f, Max:%f", render.m_depth_bounds_min, render.m_depth_bounds_max));
	LIST_SETTINGS_ADD("Depth func", !(render.m_set_depth_func) ? "(none)" : wxString::Format("0x%x (%s)",
		render.m_depth_func,
		ParseGCMEnum(render.m_depth_func, CELL_GCM_ENUM)));
	LIST_SETTINGS_ADD("Draw mode", wxString::Format("%d (%s)",
		render.m_draw_mode,
		ParseGCMEnum(render.m_draw_mode, CELL_GCM_PRIMITIVE_ENUM)));
	LIST_SETTINGS_ADD("Scissor", wxString::Format("X:%d, Y:%d, W:%d, H:%d",
		render.m_scissor_x,
		render.m_scissor_y,
		render.m_scissor_w,
		render.m_scissor_h));
	LIST_SETTINGS_ADD("Stencil func", !(render.m_set_stencil_func) ? "(none)" : wxString::Format("0x%x (%s)",
		render.m_stencil_func,
		ParseGCMEnum(render.m_stencil_func, CELL_GCM_ENUM)));
	LIST_SETTINGS_ADD("Surface Pitch A", wxString::Format("0x%x", render.m_surface_pitch_a));
	LIST_SETTINGS_ADD("Surface Pitch B", wxString::Format("0x%x", render.m_surface_pitch_b));
	LIST_SETTINGS_ADD("Surface Pitch C", wxString::Format("0x%x", render.m_surface_pitch_c));
	LIST_SETTINGS_ADD("Surface Pitch D", wxString::Format("0x%x", render.m_surface_pitch_d));
	LIST_SETTINGS_ADD("Surface Pitch Z", wxString::Format("0x%x", render.m_surface_pitch_z));
	LIST_SETTINGS_ADD("Surface Offset A", wxString::Format("0x%x", render.m_surface_offset_a));
	LIST_SETTINGS_ADD("Surface Offset B", wxString::Format("0x%x", render.m_surface_offset_b));
	LIST_SETTINGS_ADD("Surface Offset C", wxString::Format("0x%x", render.m_surface_offset_c));
	LIST_SETTINGS_ADD("Surface Offset D", wxString::Format("0x%x", render.m_surface_offset_d));
	LIST_SETTINGS_ADD("Surface Offset Z", wxString::Format("0x%x", render.m_surface_offset_z));
	LIST_SETTINGS_ADD("Viewport", wxString::Format("X:%d, Y:%d, W:%d, H:%d",
		render.m_viewport_x,
		render.m_viewport_y,
		render.m_viewport_w,
		render.m_viewport_h));

#undef LIST_SETTINGS_ADD
}

void RSXDebugger::SetFlags(wxListEvent& event)
{
	if (!RSXReady()) return;
	GSRender& render = Emu.GetGSManager().GetRender();
	switch(event.m_itemIndex)
	{
	case 0:  render.m_set_alpha_test		^= true; break;
	case 1:  render.m_set_blend			^= true; break;
	case 2:  render.m_set_cull_face			^= true; break;
	case 3:  render.m_set_depth_bounds_test		^= true; break;
	case 4:  render.m_set_depth_test		^= true; break;
	case 5:  render.m_set_dither			^= true; break;
	case 6:  render.m_set_line_smooth		^= true; break;
	case 7:  render.m_set_logic_op			^= true; break;
	case 8:  render.m_set_poly_smooth		^= true; break;
	case 9:  render.m_set_poly_offset_fill		^= true; break;
	case 10: render.m_set_poly_offset_line		^= true; break;
	case 11: render.m_set_poly_offset_point		^= true; break;
	case 12: render.m_set_stencil_test		^= true; break;
	case 13: render.m_set_point_sprite_control	^= true; break;
	case 14: render.m_set_restart_index		^= true; break;
	case 15: render.m_set_specular			^= true; break;
	case 16: render.m_set_scissor_horizontal	^= true; break;
	case 17: render.m_set_scissor_vertical		^= true; break;
	}

	UpdateInformation();
}

void RSXDebugger::SetPrograms(wxListEvent& event)
{
	if (!RSXReady()) return;
	GSRender& render = Emu.GetGSManager().GetRender();
	RSXDebuggerProgram& program = m_debug_programs[event.m_itemIndex];

	// Program Editor
	wxString title = wxString::Format("Program ID: %d (VP:%d, FP:%d)", program.id, program.vp_id, program.fp_id);
	wxDialog d_editor(this, wxID_ANY, title, wxDefaultPosition, wxSize(800,500),
		wxDEFAULT_DIALOG_STYLE | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxRESIZE_BORDER);

	wxBoxSizer& s_panel = *new wxBoxSizer(wxHORIZONTAL);
	wxStaticBoxSizer& s_vp_box = *new wxStaticBoxSizer(wxHORIZONTAL, &d_editor, "Vertex Program");
	wxStaticBoxSizer& s_fp_box = *new wxStaticBoxSizer(wxHORIZONTAL, &d_editor, "Fragment Program");
	wxTextCtrl* t_vp_edit  = new wxTextCtrl(&d_editor, wxID_ANY, program.vp_shader, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	wxTextCtrl* t_fp_edit  = new wxTextCtrl(&d_editor, wxID_ANY, program.fp_shader, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	t_vp_edit->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	t_fp_edit->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	s_vp_box.Add(t_vp_edit, 1, wxEXPAND);
	s_fp_box.Add(t_fp_edit, 1, wxEXPAND);
	s_panel.Add(&s_vp_box, 1, wxEXPAND);
	s_panel.Add(&s_fp_box, 1, wxEXPAND);
	d_editor.SetSizer(&s_panel);

	// Show editor and open Save Dialog when closing
	if (d_editor.ShowModal())
	{
		wxMessageDialog d_save(&d_editor, "Save changes and compile shaders?", title, wxYES_NO|wxCENTRE);
		if(d_save.ShowModal() == wxID_YES)
		{
			program.modified = true;
			program.vp_shader = t_vp_edit->GetValue();
			program.fp_shader = t_fp_edit->GetValue();
		}
	}
	UpdateInformation();
}

void RSXDebugger::OnSelectTexture(wxListEvent& event)
{
	if(event.GetIndex() >= 0)
		m_cur_texture = event.GetIndex();

	UpdateInformation();
}

const char* RSXDebugger::ParseGCMEnum(u32 value, u32 type)
{
	switch(type)
	{
	case CELL_GCM_ENUM:
	{
		switch(value)
		{
		case 0x0200: return "Never";
		case 0x0201: return "Less";
		case 0x0202: return "Equal";
		case 0x0203: return "Less or Equal";
		case 0x0204: return "Greater";
		case 0x0205: return "Not Equal";
		case 0x0206: return "Greater or Equal";
		case 0x0207: return "Always";

		case 0x0:    return "Zero";
		case 0x1:    return "One";
		case 0x0300: return "SRC_COLOR";
		case 0x0301: return "1 - SRC_COLOR";
		case 0x0302: return "SRC_ALPHA";
		case 0x0303: return "1 - SRC_ALPHA";
		case 0x0304: return "DST_ALPHA";
		case 0x0305: return "1 - DST_ALPHA";
		case 0x0306: return "DST_COLOR";
		case 0x0307: return "1 - DST_COLOR";
		case 0x0308: return "SRC_ALPHA_SATURATE";
		case 0x8001: return "CONSTANT_COLOR";
		case 0x8002: return "1 - CONSTANT_COLOR";
		case 0x8003: return "CONSTANT_ALPHA";
		case 0x8004: return "1 - CONSTANT_ALPHA";

		case 0x8006: return "Add";
		case 0x8007: return "Min";
		case 0x8008: return "Max";
		case 0x800A: return "Substract";
		case 0x800B: return "Reverse Substract";
		case 0xF005: return "Reverse Substract Signed";
		case 0xF006: return "Add Signed";
		case 0xF007: return "Reverse Add Signed";

		default: return "Wrong Value!";
		}
	}
	case CELL_GCM_PRIMITIVE_ENUM:
	{
		switch(value)
		{
		case 1:  return "POINTS";
		case 2:  return "LINES";
		case 3:  return "LINE_LOOP";
		case 4:  return "LINE_STRIP";
		case 5:  return "TRIANGLES";
		case 6:  return "TRIANGLE_STRIP";
		case 7:  return "TRIANGLE_FAN";
		case 8:  return "QUADS";
		case 9:  return "QUAD_STRIP";
		case 10: return "POLYGON";

		default: return "Wrong Value!"; 
		}
	}
	default: return "Unknown!";
	}
}

#define case_16(a, m) \
	case a + m: \
	case a + m * 2: \
	case a + m * 3: \
	case a + m * 4: \
	case a + m * 5: \
	case a + m * 6: \
	case a + m * 7: \
	case a + m * 8: \
	case a + m * 9: \
	case a + m * 10: \
	case a + m * 11: \
	case a + m * 12: \
	case a + m * 13: \
	case a + m * 14: \
	case a + m * 15: \
	index = (cmd - a) / m; \
	case a \

wxString RSXDebugger::DisAsmCommand(u32 cmd, u32 count, u32 currentAddr, u32 ioAddr)
{
	wxString disasm = wxEmptyString;

#define DISASM(string, ...) { if(disasm.IsEmpty()) disasm = wxString::Format((string), ##__VA_ARGS__); else disasm += (wxString(' ') + wxString::Format((string), ##__VA_ARGS__)); }
	if(cmd & CELL_GCM_METHOD_FLAG_JUMP)
	{
		u32 jumpAddr = cmd & ~(CELL_GCM_METHOD_FLAG_JUMP | CELL_GCM_METHOD_FLAG_NON_INCREMENT);
		DISASM("JUMP: %08x -> %08x", currentAddr, ioAddr+jumpAddr);
	}
	else if(cmd & CELL_GCM_METHOD_FLAG_CALL)
	{
		u32 callAddr = cmd & ~CELL_GCM_METHOD_FLAG_CALL;
		DISASM("CALL: %08x -> %08x", currentAddr, ioAddr+callAddr);
	}
	if(cmd == CELL_GCM_METHOD_FLAG_RETURN)
	{
		DISASM("RETURN");
	}

	if(cmd == 0)
	{
		DISASM("Null cmd");
	}
	else if(!(cmd & (CELL_GCM_METHOD_FLAG_JUMP | CELL_GCM_METHOD_FLAG_CALL)) && cmd != CELL_GCM_METHOD_FLAG_RETURN)
	{
		auto args = vm::ptr<u32>::make(currentAddr + 4);

		u32 index = 0;
		switch(cmd & 0x3ffff)
		{
		case NV406E_SEMAPHORE_OFFSET:
			DISASM("PFIFO: Semaphore offset 0x%x", (u32)args[0]);
			break;

		case NV406E_SEMAPHORE_ACQUIRE:
			DISASM("PFIFO: Semaphore acquire at 0x%x", (u32)args[0]);
			break;

		case NV406E_SEMAPHORE_RELEASE:
			DISASM("PFIFO: Semaphore release value 0x%x", (u32)args[0]);
			break;

		case NV4097_SET_SURFACE_FORMAT:
		{
			const u32 a0 = (u32)args[0];
			const u32 surface_format = a0 & 0x1f;
			const u32 surface_depth_format = (a0 >> 5) & 0x7;

			const char *depth_type_name, *color_type_name;
			switch (surface_depth_format)
			{
			case CELL_GCM_SURFACE_Z16:
				depth_type_name = "CELL_GCM_SURFACE_Z16";
				break;
			case CELL_GCM_SURFACE_Z24S8:
				depth_type_name = "CELL_GCM_SURFACE_Z24S8";
				break;
			default: depth_type_name = "";
				break;
			}
			switch (surface_format)
			{
			case CELL_GCM_SURFACE_X1R5G5B5_Z1R5G5B5:
				color_type_name = "CELL_GCM_SURFACE_X1R5G5B5_Z1R5G5B5";
				break;
			case CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5:
				color_type_name = "CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5";
				break;
			case CELL_GCM_SURFACE_R5G6B5:
				color_type_name = "CELL_GCM_SURFACE_R5G6B5";
				break;
			case CELL_GCM_SURFACE_X8R8G8B8_Z8R8G8B8:
				color_type_name = "CELL_GCM_SURFACE_X8R8G8B8_Z8R8G8B8";
				break;
			case CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8:
				color_type_name = "CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8";
				break;
			case CELL_GCM_SURFACE_A8R8G8B8:
				color_type_name = "CELL_GCM_SURFACE_A8R8G8B8";
				break;
			case CELL_GCM_SURFACE_B8:
				color_type_name = "CELL_GCM_SURFACE_B8";
				break;
			case CELL_GCM_SURFACE_G8B8:
				color_type_name = "CELL_GCM_SURFACE_G8B8";
				break;
			case CELL_GCM_SURFACE_F_W16Z16Y16X16:
				color_type_name = "CELL_GCM_SURFACE_F_W16Z16Y16X16";
				break;
			case CELL_GCM_SURFACE_F_W32Z32Y32X32:
				color_type_name = "CELL_GCM_SURFACE_F_W32Z32Y32X32";
				break;
			case CELL_GCM_SURFACE_F_X32:
				color_type_name = "CELL_GCM_SURFACE_F_X32";
				break;
			case CELL_GCM_SURFACE_X8B8G8R8_Z8B8G8R8:
				color_type_name = "CELL_GCM_SURFACE_X8B8G8R8_Z8B8G8R8";
				break;
			case CELL_GCM_SURFACE_X8B8G8R8_O8B8G8R8:
				color_type_name = "CELL_GCM_SURFACE_X8B8G8R8_O8B8G8R8";
				break;
			case CELL_GCM_SURFACE_A8B8G8R8:
				color_type_name = "CELL_GCM_SURFACE_A8B8G8R8";
				break;
			default: color_type_name = "";
				break;
			}
			DISASM("Set surface format : C %s Z %s", color_type_name, depth_type_name);
		}
			break;

		case NV4097_SET_SURFACE_COLOR_TARGET:
			DISASM("Set surface color target");
			break;

		case NV4097_SET_SHADER_WINDOW:
			DISASM("Set shader windows");
			break;

		case NV4097_SET_DEPTH_TEST_ENABLE:
			DISASM("Set depth test enable");
			break;

		case NV4097_SET_DEPTH_FUNC:
			DISASM("Set depth func");
			break;

		case NV4097_SET_ZSTENCIL_CLEAR_VALUE:
			DISASM("Set ZSTENCIL clear value");
			break;

		case NV4097_CLEAR_SURFACE:
			DISASM("Clear surface");
			break;

		case NV4097_SET_TRANSFORM_CONSTANT_LOAD:
			DISASM("Set transform constant load");
			break;

		case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT:
			DISASM("Set vertex data array format");
			break;

		case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET:
			DISASM("Set vertex data array offset");
			break;

		case NV4097_SET_SHADER_PROGRAM:
			DISASM("Set shader program");
			break;

		case NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK:
			DISASM("Set vertex attrib output mask");
			break;

		case NV4097_SET_TEX_COORD_CONTROL:
			DISASM("Set tex coord control");
			break;

		case NV4097_SET_TRANSFORM_PROGRAM_LOAD:
			DISASM("Set transform program load");
			break;

		case NV4097_SET_TRANSFORM_PROGRAM:
			DISASM("Set transform program");
			break;

		case NV4097_SET_VERTEX_ATTRIB_INPUT_MASK:
			DISASM("Set vertex attrib input mask");
			break;

		case NV4097_SET_TRANSFORM_TIMEOUT:
			DISASM("Set transform timeout");
			break;

		case NV4097_INVALIDATE_VERTEX_CACHE_FILE:
			DISASM("Invalidate vertex cache file");
			break;

		case NV4097_SET_SHADER_CONTROL:
			DISASM("Set shader control");
			break;

		case NV4097_SET_SEMAPHORE_OFFSET:
			DISASM("PGRAPH: Set semaphore offset 0x%x", (u32)args[0]);
			break;

		case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
			DISASM("PGRAPH: Back end write semaphore release %x", (u32)args[0]);
			break;

		case NV4097_SET_COLOR_MASK_MRT:
			DISASM("Set color mask MRT");
			break;

		case NV4097_SET_TEXTURE_IMAGE_RECT:
			DISASM("Set texture image rect");
			break;

		case NV4097_SET_TEXTURE_CONTROL3:
			DISASM("Set texture control 3");
			break;

		case NV4097_SET_TEXTURE_CONTROL1:
			DISASM("Set texture control 1");
			break;

		case NV4097_SET_TEXTURE_CONTROL0:
			DISASM("Set texture control 0");
			break;

		case NV4097_SET_TEXTURE_ADDRESS:
			DISASM("Set texture address");
			break;

		case NV4097_SET_TEXTURE_FILTER:
			DISASM("Set texture filter");
			break;

		case NV4097_SET_BLEND_FUNC_SFACTOR:
			DISASM("Set blend func sfactor");
			break;

		case NV4097_SET_FRONT_POLYGON_MODE:
			DISASM("Set front polygon mode");
			break;

		case NV4097_SET_VIEWPORT_HORIZONTAL:
		{
			u32 m_viewport_x = (u32)args[0] & 0xffff;
			u32 m_viewport_w = (u32)args[0] >> 16;

			if (count == 2)
			{
				u32 m_viewport_y = (u32)args[1] & 0xffff;
				u32 m_viewport_h = (u32)args[1] >> 16;
				DISASM("Set viewport horizontal %d %d", m_viewport_w, m_viewport_h);
			}
			else
				DISASM("Set viewport horizontal %d", m_viewport_w);
			break;
		}

		case NV4097_SET_CLIP_MIN:
			DISASM("Set clip min");
			break;

		case NV4097_SET_VIEWPORT_OFFSET:
			DISASM("Set viewport offset");
			break;

		case NV4097_SET_SCISSOR_HORIZONTAL:
			DISASM("Set scissor horizontal");
			break;

		case NV4097_INVALIDATE_L2:
			DISASM("Invalidate L2");
			break;

		case NV4097_INVALIDATE_VERTEX_FILE:
			DISASM("Invalidate vertex file");
			break;

		case NV4097_SET_BEGIN_END:
			DISASM("Set BEGIN END");
			break;

		case NV4097_DRAW_ARRAYS:
			DISASM("Draw arrays");
			break;

		case NV4097_SET_WINDOW_OFFSET:
			DISASM("Set window offset");
			break;

		case NV4097_SET_SURFACE_CLIP_HORIZONTAL:
		{
			const u32 a0 = (u32)args[0];

			u32 clip_x = a0;
			u32 clip_w = a0 >> 16;

			if (count == 2)
			{
				const u32 a1 = (u32)args[1];
				u32 clip_y = a1;
				u32 clip_h = a1 >> 16;
				DISASM("Set surface clip horizontal : %d %d", clip_w, clip_h);
			}
			else
				DISASM("Set surface clip horizontal : %d", clip_w);
			break;
		}

			break;

		case 0x3fead:
			DISASM("Flip and change current buffer: %d", (u32)args[0]);
		break;

		case NV4097_NO_OPERATION:
			DISASM("NOP");
		break;

		case NV406E_SET_REFERENCE:
			DISASM("Set reference: 0x%x", (u32)args[0]);
		break;

		case_16(NV4097_SET_TEXTURE_OFFSET, 0x20):
			DISASM("Texture Offset[%d]: %08x", index, (u32)args[0]);
			switch ((args[1] & 0x3) - 1)
			{
			case CELL_GCM_LOCATION_LOCAL: DISASM("(Local memory);");  break;
			case CELL_GCM_LOCATION_MAIN:  DISASM("(Main memory);");   break;
			default:                      DISASM("(Bad location!);"); break;
			}
			DISASM("    Cubemap:%s; Dimension:0x%x; Format:0x%x; Mipmap:0x%x",
				(((args[1] >> 2) & 0x1) ? "True" : "False"),
				((args[1] >> 4) & 0xf),
				((args[1] >> 8) & 0xff),
				((args[1] >> 16) & 0xffff));
		break;

		case NV4097_SET_COLOR_MASK:
			DISASM("    Color mask: True (A:%c, R:%c, G:%c, B:%c)",
				args[0] & 0x1000000 ? '1' : '0',
				args[0] & 0x0010000 ? '1' : '0',
				args[0] & 0x0000100 ? '1' : '0',
				args[0] & 0x0000001 ? '1' : '0');
		break;

		case NV4097_SET_ALPHA_TEST_ENABLE:
			DISASM(args[0] ? "Alpha test: Enable" : "Alpha test: Disable");
		break;

		case NV4097_SET_BLEND_ENABLE:
			DISASM(args[0] ? "Blend: Enable" : "Blend: Disable");
		break;

		case NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE:
			DISASM(args[0] ? "Depth bounds test: Enable" : "Depth bounds test: Disable");
		break;

		case NV4097_SET_CONTEXT_DMA_COLOR_A:
			DISASM("Context DMA Color A: 0x%x", (u32)args[0]);
		break;

		case NV4097_SET_CONTEXT_DMA_COLOR_B:
			DISASM("Context DMA Color B: 0x%x", (u32)args[0]);
		break;

		case NV4097_SET_CONTEXT_DMA_COLOR_C:
			DISASM("Context DMA Color C: 0x%x", (u32)args[0]);
			if(count > 1)
				DISASM("0x%x", (u32)args[1]);
		break;

		case NV4097_SET_CONTEXT_DMA_ZETA:
			DISASM("Context DMA Zeta: 0x%x", (u32)args[0]);
		break;

		case NV4097_SET_SURFACE_PITCH_C:
			DISASM("Surface Pitch C: 0x%x;", (u32)args[0]);
			DISASM("Surface Pitch D: 0x%x;", (u32)args[1]);
			DISASM("Surface Offset C: 0x%x;", (u32)args[2]);
			DISASM("Surface Offset D: 0x%x", (u32)args[3]);
		break;

		case NV4097_SET_SURFACE_PITCH_Z:
			DISASM("Surface Pitch Z: 0x%x;", (u32)args[0]);
		break;

		default:
			break;
		}

		if(cmd & CELL_GCM_METHOD_FLAG_NON_INCREMENT)
		{
			DISASM("Non Increment cmd");
		}

		DISASM("[0x%08x(", cmd);

		for(uint i=0; i<count; ++i)
		{
			if(i != 0) disasm += ", ";
			disasm += wxString::Format("0x%x", (u32)args[i]);
		}

		disasm += ")]";
	}
#undef DISASM

	return disasm;
}

bool RSXDebugger::RSXReady()
{
	return Emu.GetGSManager().IsInited();
}
