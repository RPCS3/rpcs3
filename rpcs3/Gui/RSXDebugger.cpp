#include "stdafx.h"
#include "stdafx_gui.h"

#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/RSX/GSRender.h"
#include "Emu/RSX/GCM.h"

#include "MemoryViewer.h"
#include "RSXDebugger.h"

#include "Utilities/GSL.h"

enum GCMEnumTypes
{
	CELL_GCM_ENUM,
	CELL_GCM_PRIMITIVE_ENUM,
};

RSXDebugger::RSXDebugger(wxWindow* parent) 
	: wxDialog(parent, wxID_ANY, "RSX Debugger", wxDefaultPosition, wxSize(700, 450))
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


	wxNotebook* nb_rsx = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxSize(732, 732));

	//Tabs
	wxPanel* p_commands  = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_captured_frame = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_captured_draw_calls = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_flags     = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_lightning = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_texture   = new wxPanel(nb_rsx, wxID_ANY);
	wxPanel* p_settings  = new wxPanel(nb_rsx, wxID_ANY);

	nb_rsx->AddPage(p_commands, "RSX Commands");
	nb_rsx->AddPage(p_captured_frame, "Captured Frame");
	nb_rsx->AddPage(p_captured_draw_calls, "Captured Draw Calls");
	nb_rsx->AddPage(p_flags, "Flags");

	nb_rsx->AddPage(p_lightning, "Lightning");
	nb_rsx->AddPage(p_texture, "Texture");
	nb_rsx->AddPage(p_settings, "Settings");

	//Tabs: Lists
	m_list_commands  = new wxListView(p_commands,  wxID_ANY, wxPoint(1,3), wxSize(720, 720));
	m_list_captured_frame = new wxListView(p_captured_frame, wxID_ANY, wxPoint(1, 3), wxSize(720, 720));
	m_list_captured_draw_calls = new wxListView(p_captured_draw_calls, wxID_ANY, wxPoint(1, 3), wxSize(720, 720));
	m_list_flags     = new wxListView(p_flags,     wxID_ANY, wxPoint(1,3), wxSize(720, 720));
	m_list_lightning = new wxListView(p_lightning, wxID_ANY, wxPoint(1,3), wxSize(720, 720));
	m_list_texture   = new wxListView(p_texture,   wxID_ANY, wxPoint(1,3), wxSize(720, 720));
	m_list_settings  = new wxListView(p_settings,  wxID_ANY, wxPoint(1,3), wxSize(720, 720));
	
	//Tabs: List Style
	m_list_commands ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_captured_frame->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_captured_draw_calls->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_flags    ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_lightning->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_texture  ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_list_settings ->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	//Tabs: List Columns
	m_list_commands->InsertColumn(0, "Address", 0, 80);
	m_list_commands->InsertColumn(1, "Value", 0, 80);
	m_list_commands->InsertColumn(2, "Command", 0, 500);
	m_list_commands->InsertColumn(3, "Count", 0, 40);
	m_list_captured_frame->InsertColumn(0, "Column", 0, 720);
	m_list_captured_draw_calls->InsertColumn(0, "Draw calls", 0, 720);
	m_list_flags->InsertColumn(0, "Name", 0, 170);
	m_list_flags->InsertColumn(1, "Value", 0, 270);
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
	for (u32 i = 0; i<frame_debug.command_queue.size(); i++)
		m_list_captured_frame->InsertItem(1, wxEmptyString);

	//Tools: Tools = Controls + Notebook Tabs
	s_tools->AddSpacer(10);
	s_tools->Add(s_controls);
	s_tools->AddSpacer(10);
	s_tools->Add(nb_rsx);
	s_tools->AddSpacer(10);

	// State explorer
	wxBoxSizer* s_state_explorer = new wxBoxSizer(wxHORIZONTAL);
	wxNotebook* state_rsx = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxSize(732, 732));

	wxPanel* p_buffers = new wxPanel(state_rsx, wxID_ANY);
	wxPanel* p_transform_program = new wxPanel(state_rsx, wxID_ANY);
	wxPanel* p_shader_program = new wxPanel(state_rsx, wxID_ANY);
	wxPanel* p_index_buffer = new wxPanel(state_rsx, wxID_ANY);

	state_rsx->AddPage(p_buffers, "RTTs and DS");
	state_rsx->AddPage(p_transform_program, "Transform program");
	state_rsx->AddPage(p_shader_program, "Shader program");
	state_rsx->AddPage(p_index_buffer, "Index buffer");

	m_text_transform_program = new wxTextCtrl(p_transform_program, wxID_ANY, "", wxPoint(1, 3), wxSize(720, 720), wxTE_MULTILINE | wxTE_READONLY);
	m_text_transform_program->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	m_text_shader_program = new wxTextCtrl(p_shader_program, wxID_ANY, "", wxPoint(1, 3), wxSize(720, 720), wxTE_MULTILINE | wxTE_READONLY);
	m_text_shader_program->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	m_list_index_buffer = new wxListView(p_index_buffer, wxID_ANY, wxPoint(1, 3), wxSize(720, 720));
	m_list_index_buffer->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	//Buffers
	wxBoxSizer* s_buffers1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_buffers2 = new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer* s_buffers_colorA  = new wxStaticBoxSizer(wxHORIZONTAL, p_buffers, "Color Buffer A");
	wxStaticBoxSizer* s_buffers_colorB  = new wxStaticBoxSizer(wxHORIZONTAL, p_buffers, "Color Buffer B");
	wxStaticBoxSizer* s_buffers_colorC  = new wxStaticBoxSizer(wxHORIZONTAL, p_buffers, "Color Buffer C");
	wxStaticBoxSizer* s_buffers_colorD  = new wxStaticBoxSizer(wxHORIZONTAL, p_buffers, "Color Buffer D");
	wxStaticBoxSizer* s_buffers_depth   = new wxStaticBoxSizer(wxHORIZONTAL, p_buffers, "Depth Buffer");
	wxStaticBoxSizer* s_buffers_stencil = new wxStaticBoxSizer(wxHORIZONTAL, p_buffers, "Stencil Buffer");
	wxStaticBoxSizer* s_buffers_text    = new wxStaticBoxSizer(wxHORIZONTAL, p_buffers, "Texture");
	
	//Buffers and textures
	m_panel_width = 108;
	m_panel_height = 108;
	m_text_width = 108;
	m_text_height = 108;

	//Panels for displaying the buffers
	p_buffer_colorA  = new wxPanel(p_buffers, wxID_ANY, wxDefaultPosition, wxSize(m_panel_width, m_panel_height));
	p_buffer_colorB  = new wxPanel(p_buffers, wxID_ANY, wxDefaultPosition, wxSize(m_panel_width, m_panel_height));
	p_buffer_colorC  = new wxPanel(p_buffers, wxID_ANY, wxDefaultPosition, wxSize(m_panel_width, m_panel_height));
	p_buffer_colorD  = new wxPanel(p_buffers, wxID_ANY, wxDefaultPosition, wxSize(m_panel_width, m_panel_height));
	p_buffer_depth   = new wxPanel(p_buffers, wxID_ANY, wxDefaultPosition, wxSize(m_panel_width, m_panel_height));
	p_buffer_stencil = new wxPanel(p_buffers, wxID_ANY, wxDefaultPosition, wxSize(m_panel_width, m_panel_height));
	p_buffer_tex     = new wxPanel(p_buffers, wxID_ANY, wxDefaultPosition, wxSize(m_text_width, m_text_height));
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

	s_state_explorer->Add(s_buffers1);
	s_state_explorer->AddSpacer(10);
	s_state_explorer->Add(s_buffers2);

	p_buffers->SetSizerAndFit(s_state_explorer);

	s_panel->AddSpacer(10);
	s_panel->Add(s_tools);
	s_panel->AddSpacer(10);
	s_panel->Add(state_rsx);
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
	p_buffer_depth->Bind(wxEVT_LEFT_DOWN, &RSXDebugger::OnClickBuffer, this);
	p_buffer_stencil->Bind(wxEVT_LEFT_DOWN, &RSXDebugger::OnClickBuffer, this);
	p_buffer_tex->Bind(wxEVT_LEFT_DOWN, &RSXDebugger::OnClickBuffer, this);
	m_list_captured_draw_calls->Bind(wxEVT_LEFT_DOWN, &RSXDebugger::OnClickDrawCalls, this);

	m_list_commands->Bind(wxEVT_MOUSEWHEEL, &RSXDebugger::OnScrollMemory, this);
	m_list_flags->Bind(wxEVT_LIST_ITEM_ACTIVATED, &RSXDebugger::SetFlags, this);

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
	if(vm::check_addr(m_addr, 4))
	{
		int items = event.ControlDown() ? m_item_count : 1;

		for(int i=0; i<items; ++i)
		{
			u32 offset;
			if(vm::check_addr(m_addr, 4))
			{
				u32 cmd = vm::ps3::read32(m_addr);
				u32 count = ((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) == RSX_METHOD_OLD_JUMP_CMD)
					|| ((cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD)
					|| ((cmd & RSX_METHOD_CALL_CMD_MASK) == RSX_METHOD_CALL_CMD)
					|| cmd == RSX_METHOD_RETURN_CMD ? 0 : (cmd >> 18) & 0x7ff;

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

namespace
{
	void display_buffer(wxWindow *parent, const wxImage &img)
	{
//		wxString title = wxString::Format("Raw Image @ 0x%x", addr);
		size_t width = img.GetWidth(), height = img.GetHeight();
		wxFrame* f_image_viewer = new wxFrame(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
			wxSYSTEM_MENU | wxMINIMIZE_BOX | wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN);
		f_image_viewer->SetBackgroundColour(wxColour(240, 240, 240)); //This fix the ugly background color under Windows
		f_image_viewer->SetAutoLayout(true);
		f_image_viewer->SetClientSize(wxSize(width, height));
		f_image_viewer->Show();
		wxClientDC dc_canvas(f_image_viewer);
		dc_canvas.DrawBitmap(img, 0, 0, false);
	}
}

void RSXDebugger::OnClickBuffer(wxMouseEvent& event)
{
	const auto render = fxm::get<GSRender>();
	if (!render)
	{
		return;
	}

	const auto buffers = render->gcm_buffers;
	if(!buffers)
		return;

	// TODO: Is there any better way to choose the color buffers
#define SHOW_BUFFER(id) \
	{ \
		u32 addr = render->local_mem_addr + buffers[id].offset; \
		if (vm::check_addr(addr) && buffers[id].width && buffers[id].height) \
			MemoryViewerPanel::ShowImage(this, addr, 3, buffers[id].width, buffers[id].height, true); \
		return; \
	} \

/*	if (event.GetId() == p_buffer_colorA->GetId()) SHOW_BUFFER(0);
	if (event.GetId() == p_buffer_colorB->GetId()) SHOW_BUFFER(1);
	if (event.GetId() == p_buffer_colorC->GetId()) SHOW_BUFFER(2);
	if (event.GetId() == p_buffer_colorD->GetId()) SHOW_BUFFER(3);*/

	if (event.GetId() == p_buffer_colorA->GetId()) display_buffer(this, buffer_img[0]);
	if (event.GetId() == p_buffer_colorB->GetId()) display_buffer(this, buffer_img[1]);
	if (event.GetId() == p_buffer_colorC->GetId()) display_buffer(this, buffer_img[2]);
	if (event.GetId() == p_buffer_colorD->GetId()) display_buffer(this, buffer_img[3]);
	if (event.GetId() == p_buffer_depth->GetId()) display_buffer(this, depth_img);
	if (event.GetId() == p_buffer_stencil->GetId()) display_buffer(this, stencil_img);
	if (event.GetId() == p_buffer_tex->GetId())
	{
/*		u8 location = render->textures[m_cur_texture].location();
		if(location <= 1 && vm::check_addr(rsx::get_address(render->textures[m_cur_texture].offset(), location))
			&& render->textures[m_cur_texture].width() && render->textures[m_cur_texture].height())
			MemoryViewerPanel::ShowImage(this,
				rsx::get_address(render->textures[m_cur_texture].offset(), location), 1,
				render->textures[m_cur_texture].width(),
				render->textures[m_cur_texture].height(), false);*/
	}

#undef SHOW_BUFFER
}

namespace
{
	std::array<u8, 3> get_value(gsl::multi_span<const gsl::byte> orig_buffer, rsx::surface_color_format format, size_t idx)
	{
		switch (format)
		{
		case rsx::surface_color_format::b8:
		{
			u8 value = gsl::as_multi_span<const u8>(orig_buffer)[idx];
			return{ value, value, value };
		}
		case rsx::surface_color_format::x32:
		{
			be_t<u32> stored_val = gsl::as_multi_span<const be_t<u32>>(orig_buffer)[idx];
			u32 swapped_val = stored_val;
			f32 float_val = (f32&)swapped_val;
			u8 val = float_val * 255.f;
			return{ val, val, val };
		}
		case rsx::surface_color_format::a8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		{
			auto ptr = gsl::as_multi_span<const u8>(orig_buffer);
			return{ ptr[1 + idx * 4], ptr[2 + idx * 4], ptr[3 + idx * 4] };
		}
		case rsx::surface_color_format::a8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		{
			auto ptr = gsl::as_multi_span<const u8>(orig_buffer);
			return{ ptr[3 + idx * 4], ptr[2 + idx * 4], ptr[1 + idx * 4] };
		}
		case rsx::surface_color_format::w16z16y16x16:
		{
			auto ptr = gsl::as_multi_span<const u16>(orig_buffer);
			f16 h0 = f16(ptr[4 * idx]);
			f16 h1 = f16(ptr[4 * idx + 1]);
			f16 h2 = f16(ptr[4 * idx + 2]);
			f32 f0 = float(h0);
			f32 f1 = float(h1);
			f32 f2 = float(h2);

			u8 val0 = f0 * 255.;
			u8 val1 = f1 * 255.;
			u8 val2 = f2 * 255.;
			return{ val0, val1, val2 };
		}
		case rsx::surface_color_format::g8b8:
		case rsx::surface_color_format::r5g6b5:
		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		case rsx::surface_color_format::w32z32y32x32:
			fmt::throw_exception("Unsupported format for display" HERE);
		}
	}

	/**
	 * Return a new buffer that can be passed to wxImage ctor.
	 * The pointer seems to be freed by wxImage.
	 */
	u8* convert_to_wximage_buffer(rsx::surface_color_format format, gsl::multi_span<const gsl::byte> orig_buffer, size_t width, size_t height) noexcept
	{
		unsigned char* buffer = (unsigned char*)malloc(width * height * 3);
		for (u32 i = 0; i < width * height; i++)
		{
			const auto &colors = get_value(orig_buffer, format, i);
			buffer[0 + i * 3] = colors[0];
			buffer[1 + i * 3] = colors[1];
			buffer[2 + i * 3] = colors[2];
		}
		return buffer;
	}
};

void RSXDebugger::OnClickDrawCalls(wxMouseEvent& event)
{
	size_t draw_id = m_list_captured_draw_calls->GetFirstSelected();

	const auto& draw_call = frame_debug.draw_calls[draw_id];

	wxPanel* p_buffers[] =
	{
		p_buffer_colorA,
		p_buffer_colorB,
		p_buffer_colorC,
		p_buffer_colorD,
	};

	size_t width = draw_call.state.surface_clip_width();
	size_t height = draw_call.state.surface_clip_height();

	for (size_t i = 0; i < 4; i++)
	{
		if (width && height && !draw_call.color_buffer[i].empty())
		{
			buffer_img[i] = wxImage(width, height, convert_to_wximage_buffer(draw_call.state.surface_color(), draw_call.color_buffer[i], width, height));
			wxClientDC dc_canvas(p_buffers[i]);

			if (buffer_img[i].IsOk())
				dc_canvas.DrawBitmap(buffer_img[i].Scale(m_panel_width, m_panel_height), 0, 0, false);
		}
	}

	// Buffer Z
	{
		if (width && height && !draw_call.depth_stencil[0].empty())
		{
			gsl::multi_span<const gsl::byte> orig_buffer = draw_call.depth_stencil[0];
			unsigned char *buffer = (unsigned char *)malloc(width * height * 3);

			if (draw_call.state.surface_depth_fmt() == rsx::surface_depth_format::z24s8)
			{
				for (u32 row = 0; row < height; row++)
				{
					for (u32 col = 0; col < width; col++)
					{
						u32 depth_val = gsl::as_multi_span<const u32>(orig_buffer)[row * width + col];
						u8 displayed_depth_val = 255 * depth_val / 0xFFFFFF;
						buffer[3 * col + 0 + width * row * 3] = displayed_depth_val;
						buffer[3 * col + 1 + width * row * 3] = displayed_depth_val;
						buffer[3 * col + 2 + width * row * 3] = displayed_depth_val;
					}
				}
			}
			else
			{
				for (u32 row = 0; row < height; row++)
				{
					for (u32 col = 0; col < width; col++)
					{
						u16 depth_val = gsl::as_multi_span<const u16>(orig_buffer)[row * width + col];
						u8 displayed_depth_val = 255 * depth_val / 0xFFFF;
						buffer[3 * col + 0 + width * row * 3] = displayed_depth_val;
						buffer[3 * col + 1 + width * row * 3] = displayed_depth_val;
						buffer[3 * col + 2 + width * row * 3] = displayed_depth_val;
					}
				}
			}

			depth_img = wxImage(width, height, buffer);
			wxClientDC dc_canvas(p_buffer_depth);

			if (depth_img.IsOk())
				dc_canvas.DrawBitmap(depth_img.Scale(m_panel_width, m_panel_height), 0, 0, false);
		}
	}

	// Buffer S
	{
		if (width && height && !draw_call.depth_stencil[1].empty())
		{
			gsl::multi_span<const gsl::byte> orig_buffer = draw_call.depth_stencil[1];
			unsigned char *buffer = (unsigned char *)malloc(width * height * 3);

			for (u32 row = 0; row < height; row++)
			{
				for (u32 col = 0; col < width; col++)
				{
					u8 stencil_val = gsl::as_multi_span<const u8>(orig_buffer)[row * width + col];
					buffer[3 * col + 0 + width * row * 3] = stencil_val;
					buffer[3 * col + 1 + width * row * 3] = stencil_val;
					buffer[3 * col + 2 + width * row * 3] = stencil_val;
				}
			}

			stencil_img = wxImage(width, height, buffer);
			wxClientDC dc_canvas(p_buffer_stencil);

			if (stencil_img.IsOk())
				dc_canvas.DrawBitmap(stencil_img.Scale(m_panel_width, m_panel_height), 0, 0, false);
		}
	}

	// Programs
	m_text_transform_program->Clear();
	m_text_transform_program->AppendText(frame_debug.draw_calls[draw_id].programs.first);
	m_text_shader_program->Clear();
	m_text_shader_program->AppendText(frame_debug.draw_calls[draw_id].programs.second);

	m_list_index_buffer->ClearAll();
	m_list_index_buffer->InsertColumn(0, "Index", 0, 700);
	if (frame_debug.draw_calls[draw_id].state.index_type() == rsx::index_array_type::u16)
	{
		u16 *index_buffer = (u16*)frame_debug.draw_calls[draw_id].index.data();
		for (u32 i = 0; i < frame_debug.draw_calls[draw_id].vertex_count; ++i)
		{
			m_list_index_buffer->InsertItem(i, std::to_string(index_buffer[i]));
		}
	}
	if (frame_debug.draw_calls[draw_id].state.index_type() == rsx::index_array_type::u32)
	{
		u32 *index_buffer = (u32*)frame_debug.draw_calls[draw_id].index.data();
		for (u32 i = 0; i < frame_debug.draw_calls[draw_id].vertex_count; ++i)
		{
			m_list_index_buffer->InsertItem(i, std::to_string(index_buffer[i]));
		}
	}
}

void RSXDebugger::GoToGet(wxCommandEvent& event)
{
	if (const auto render = fxm::get<GSRender>())
	{
		u32 realAddr;
		if (RSXIOMem.getRealAddr(render->ctrl->get.load(), realAddr))
		{
			m_addr = realAddr;
			t_addr->SetValue(wxString::Format("%08x", m_addr));
			UpdateInformation();
			event.Skip();
		}
	}
}

void RSXDebugger::GoToPut(wxCommandEvent& event)
{
	if (const auto render = fxm::get<GSRender>())
	{
		u32 realAddr;
		if (RSXIOMem.getRealAddr(render->ctrl->put.load(), realAddr))
		{
			m_addr = realAddr;
			t_addr->SetValue(wxString::Format("%08x", m_addr));
			UpdateInformation();
			event.Skip();
		}
	}
}

void RSXDebugger::UpdateInformation()
{
	t_addr->SetValue(wxString::Format("%08x", m_addr));
	GetMemory();
	GetBuffers();
	GetFlags();
	GetLightning();
	GetTexture();
	GetSettings();
}

void RSXDebugger::GetMemory()
{
	// Clean commands column
	for(u32 i=0; i<m_item_count; i++)
		m_list_commands->SetItem(i, 2, wxEmptyString);

	// Write information
	for(u32 i=0, addr = m_addr; i<m_item_count; i++, addr += 4)
	{
		m_list_commands->SetItem(i, 0, wxString::Format("%08x", addr));
	
		if (vm::check_addr(addr))
		{
			u32 cmd = vm::ps3::read32(addr);
			u32 count = (cmd >> 18) & 0x7ff;
			m_list_commands->SetItem(i, 1, wxString::Format("%08x", cmd));
			m_list_commands->SetItem(i, 3, wxString::Format("%d", count));
			m_list_commands->SetItem(i, 2, DisAsmCommand(cmd, count, addr, 0));

			if((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) != RSX_METHOD_OLD_JUMP_CMD
				&& (cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) != RSX_METHOD_NEW_JUMP_CMD
				&& (cmd & RSX_METHOD_CALL_CMD_MASK) != RSX_METHOD_CALL_CMD
				&& cmd != RSX_METHOD_RETURN_CMD)
			{
				addr += 4 * count;
			}
		}
		else
		{
			m_list_commands->SetItem(i, 1, "????????");
		}
	}

	std::string dump;

	for (u32 i = 0; i < frame_debug.command_queue.size(); i++)
	{
		const std::string& str = rsx::get_pretty_printing_function(frame_debug.command_queue[i].first)(frame_debug.command_queue[i].second);
		m_list_captured_frame->SetItem(i, 0, str);

		dump += str;
		dump += '\n';
	}

	fs::file(fs::get_config_dir() + "command_dump.log", fs::rewrite).write(dump);

	for (u32 i = 0;i < frame_debug.draw_calls.size(); i++)
		m_list_captured_draw_calls->InsertItem(i, frame_debug.draw_calls[i].name);
}

void RSXDebugger::GetBuffers()
{
	const auto render = fxm::get<GSRender>();
	if (!render)
	{
		return;
	}

	// Draw Buffers
	// TODO: Currently it only supports color buffers
	for (u32 bufferId=0; bufferId < render->gcm_buffers_count; bufferId++)
	{
		if(!vm::check_addr(render->gcm_buffers.addr()))
			continue;

		auto buffers = render->gcm_buffers;
		u32 RSXbuffer_addr = render->local_mem_addr + buffers[bufferId].offset;

		if(!vm::check_addr(RSXbuffer_addr))
			continue;

		auto RSXbuffer = vm::ps3::_ptr<u8>(RSXbuffer_addr);
		
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
/*	if(!render->textures[m_cur_texture].enabled())
		return;

	u32 offset = render->textures[m_cur_texture].offset();

	if(!offset)
		return;

	u8 location = render->textures[m_cur_texture].location();

	if(location > 1)
		return;

	u32 TexBuffer_addr = rsx::get_address(offset, location);

	if(!vm::check_addr(TexBuffer_addr))
		return;

	unsigned char* TexBuffer = vm::ps3::_ptr<u8>(TexBuffer_addr);

	u32 width  = render->textures[m_cur_texture].width();
	u32 height = render->textures[m_cur_texture].height();
	unsigned char* buffer = (unsigned char*)malloc(width * height * 3);
	std::memcpy(buffer, vm::base(TexBuffer_addr), width * height * 3);

	wxImage img(width, height, buffer);
	wxClientDC dc_canvas(p_buffer_tex);
	dc_canvas.DrawBitmap(img.Scale(m_text_width, m_text_height), 0, 0, false);*/
}

void RSXDebugger::GetFlags()
{
	const auto render = fxm::get<GSRender>();
	if (!render)
	{
		return;
	}

	m_list_flags->DeleteAllItems();
	int i=0;

#define LIST_FLAGS_ADD(name, value) \
	m_list_flags->InsertItem(i, name); m_list_flags->SetItem(i, 1, value ? "Enabled" : "Disabled"); i++;
	/*
	LIST_FLAGS_ADD("Alpha test",         render->m_set_alpha_test);
	LIST_FLAGS_ADD("Blend",              render->m_set_blend);
	LIST_FLAGS_ADD("Scissor",            render->m_set_scissor_horizontal && render->m_set_scissor_vertical);
	LIST_FLAGS_ADD("Cull face",          render->m_set_cull_face);
	LIST_FLAGS_ADD("Depth bounds test",  render->m_set_depth_bounds_test);
	LIST_FLAGS_ADD("Depth test",         render->m_set_depth_test);
	LIST_FLAGS_ADD("Dither",             render->m_set_dither);
	LIST_FLAGS_ADD("Line smooth",        render->m_set_line_smooth);
	LIST_FLAGS_ADD("Logic op",           render->m_set_logic_op);
	LIST_FLAGS_ADD("Poly smooth",        render->m_set_poly_smooth);
	LIST_FLAGS_ADD("Poly offset fill",   render->m_set_poly_offset_fill);
	LIST_FLAGS_ADD("Poly offset line",   render->m_set_poly_offset_line);
	LIST_FLAGS_ADD("Poly offset point",  render->m_set_poly_offset_point);
	LIST_FLAGS_ADD("Stencil test",       render->m_set_stencil_test);
	LIST_FLAGS_ADD("Primitive restart",  render->m_set_restart_index);
	LIST_FLAGS_ADD("Two sided lighting", render->m_set_two_side_light_enable);
	LIST_FLAGS_ADD("Point Sprite",	     render->m_set_point_sprite_control);
	LIST_FLAGS_ADD("Lighting ",	         render->m_set_specular);
	*/

#undef LIST_FLAGS_ADD
}

void RSXDebugger::GetLightning()
{
	const auto render = fxm::get<GSRender>();
	if (!render)
	{
		return;
	}

	m_list_lightning->DeleteAllItems();
	int i=0;

#define LIST_LIGHTNING_ADD(name, value) \
	m_list_lightning->InsertItem(i, name); m_list_lightning->SetItem(i, 1, value); i++;

	//LIST_LIGHTNING_ADD("Shade model", (render->m_shade_mode == 0x1D00) ? "Flat" : "Smooth");

#undef LIST_LIGHTNING_ADD
}

void RSXDebugger::GetTexture()
{
	const auto render = fxm::get<GSRender>();
	if (!render)
	{
		return;
	}

	m_list_texture->DeleteAllItems();

	for(uint i=0; i<rsx::limits::fragment_textures_count; ++i)
	{
/*		if(render->textures[i].enabled())
		{
			m_list_texture->InsertItem(i, wxString::Format("%d", i));
			u8 location = render->textures[i].location();
			if(location > 1)
			{
				m_list_texture->SetItem(i, 1,
					wxString::Format("Bad address (offset=0x%x, location=%d)", render->textures[i].offset(), location));
			}
			else
			{
				m_list_texture->SetItem(i, 1, wxString::Format("0x%x", rsx::get_address(render->textures[i].offset(), location)));
			}

			m_list_texture->SetItem(i, 2, render->textures[i].cubemap() ? "True" : "False");
			m_list_texture->SetItem(i, 3, wxString::Format("%dD", render->textures[i].dimension()));
			m_list_texture->SetItem(i, 4, render->textures[i].enabled() ? "True" : "False");
			m_list_texture->SetItem(i, 5, wxString::Format("0x%x", render->textures[i].format()));
			m_list_texture->SetItem(i, 6, wxString::Format("0x%x", render->textures[i].mipmap()));
			m_list_texture->SetItem(i, 7, wxString::Format("0x%x", render->textures[i].pitch()));
			m_list_texture->SetItem(i, 8, wxString::Format("%dx%d",
				render->textures[i].width(),
				render->textures[i].height()));

			m_list_texture->SetItemBackgroundColour(i, wxColour(m_cur_texture == i ? "Wheat" : "White"));
		}*/
	}
}

void RSXDebugger::GetSettings()
{
	const auto render = fxm::get<GSRender>();
	if (!render)
	{
		return;
	}

	m_list_settings->DeleteAllItems();
	int i=0;

#define LIST_SETTINGS_ADD(name, value) \
	m_list_settings->InsertItem(i, name); m_list_settings->SetItem(i, 1, value); i++;
	/*
	LIST_SETTINGS_ADD("Alpha func", !(render->m_set_alpha_func) ? "(none)" : wxString::Format("0x%x (%s)",
		render->m_alpha_func,
		ParseGCMEnum(render->m_alpha_func, CELL_GCM_ENUM)));
	LIST_SETTINGS_ADD("Blend color", !(render->m_set_blend_color) ? "(none)" : wxString::Format("R:%d, G:%d, B:%d, A:%d",
		render->m_blend_color_r,
		render->m_blend_color_g,
		render->m_blend_color_b,
		render->m_blend_color_a));
	LIST_SETTINGS_ADD("Clipping", wxString::Format("Min:%f, Max:%f", render->m_clip_min, render->m_clip_max));
	LIST_SETTINGS_ADD("Color mask", !(render->m_set_color_mask) ? "(none)" : wxString::Format("R:%d, G:%d, B:%d, A:%d",
		render->m_color_mask_r,
		render->m_color_mask_g,
		render->m_color_mask_b,
		render->m_color_mask_a));
	LIST_SETTINGS_ADD("Context DMA Color A", wxString::Format("0x%x", render->m_context_dma_color_a));
	LIST_SETTINGS_ADD("Context DMA Color B", wxString::Format("0x%x", render->m_context_dma_color_b));
	LIST_SETTINGS_ADD("Context DMA Color C", wxString::Format("0x%x", render->m_context_dma_color_c));
	LIST_SETTINGS_ADD("Context DMA Color D", wxString::Format("0x%x", render->m_context_dma_color_d));
	LIST_SETTINGS_ADD("Context DMA Zeta", wxString::Format("0x%x", render->m_context_dma_z));
	LIST_SETTINGS_ADD("Depth bounds", wxString::Format("Min:%f, Max:%f", render->m_depth_bounds_min, render->m_depth_bounds_max));
	LIST_SETTINGS_ADD("Depth func", !(render->m_set_depth_func) ? "(none)" : wxString::Format("0x%x (%s)",
		render->m_depth_func,
		ParseGCMEnum(render->m_depth_func, CELL_GCM_ENUM)));
	LIST_SETTINGS_ADD("Draw mode", wxString::Format("%d (%s)",
		render->m_draw_mode,
		ParseGCMEnum(render->m_draw_mode, CELL_GCM_PRIMITIVE_ENUM)));
	LIST_SETTINGS_ADD("Scissor", wxString::Format("X:%d, Y:%d, W:%d, H:%d",
		render->m_scissor_x,
		render->m_scissor_y,
		render->m_scissor_w,
		render->m_scissor_h));
	LIST_SETTINGS_ADD("Stencil func", !(render->m_set_stencil_func) ? "(none)" : wxString::Format("0x%x (%s)",
		render->m_stencil_func,
		ParseGCMEnum(render->m_stencil_func, CELL_GCM_ENUM)));
	LIST_SETTINGS_ADD("Surface Pitch A", wxString::Format("0x%x", render->m_surface_pitch_a));
	LIST_SETTINGS_ADD("Surface Pitch B", wxString::Format("0x%x", render->m_surface_pitch_b));
	LIST_SETTINGS_ADD("Surface Pitch C", wxString::Format("0x%x", render->m_surface_pitch_c));
	LIST_SETTINGS_ADD("Surface Pitch D", wxString::Format("0x%x", render->m_surface_pitch_d));
	LIST_SETTINGS_ADD("Surface Pitch Z", wxString::Format("0x%x", render->m_surface_pitch_z));
	LIST_SETTINGS_ADD("Surface Offset A", wxString::Format("0x%x", render->m_surface_offset_a));
	LIST_SETTINGS_ADD("Surface Offset B", wxString::Format("0x%x", render->m_surface_offset_b));
	LIST_SETTINGS_ADD("Surface Offset C", wxString::Format("0x%x", render->m_surface_offset_c));
	LIST_SETTINGS_ADD("Surface Offset D", wxString::Format("0x%x", render->m_surface_offset_d));
	LIST_SETTINGS_ADD("Surface Offset Z", wxString::Format("0x%x", render->m_surface_offset_z));
	LIST_SETTINGS_ADD("Viewport", wxString::Format("X:%d, Y:%d, W:%d, H:%d",
		render->m_viewport_x,
		render->m_viewport_y,
		render->m_viewport_w,
		render->m_viewport_h));
		*/
#undef LIST_SETTINGS_ADD
}

void RSXDebugger::SetFlags(wxListEvent& event)
{
	/*
	if (!RSXReady()) return;
	GSRender& render = Emu.GetGSManager().GetRender();
	switch(event.m_itemIndex)
	{
	case 0:  render->m_set_alpha_test		^= true; break;
	case 1:  render->m_set_blend			^= true; break;
	case 2:  render->m_set_cull_face			^= true; break;
	case 3:  render->m_set_depth_bounds_test		^= true; break;
	case 4:  render->m_set_depth_test		^= true; break;
	case 5:  render->m_set_dither			^= true; break;
	case 6:  render->m_set_line_smooth		^= true; break;
	case 7:  render->m_set_logic_op			^= true; break;
	case 8:  render->m_set_poly_smooth		^= true; break;
	case 9:  render->m_set_poly_offset_fill		^= true; break;
	case 10: render->m_set_poly_offset_line		^= true; break;
	case 11: render->m_set_poly_offset_point		^= true; break;
	case 12: render->m_set_stencil_test		^= true; break;
	case 13: render->m_set_point_sprite_control	^= true; break;
	case 14: render->m_set_restart_index		^= true; break;
	case 15: render->m_set_specular			^= true; break;
	case 16: render->m_set_scissor_horizontal	^= true; break;
	case 17: render->m_set_scissor_vertical		^= true; break;
	}
	*/

	UpdateInformation();
}

void RSXDebugger::SetPrograms(wxListEvent& event)
{
	const auto render = fxm::get<GSRender>();
	if (!render)
	{
		return;
	}

	return;
	//RSXDebuggerProgram& program = m_debug_programs[event.m_itemIndex];

	//// Program Editor
	//wxString title = wxString::Format("Program ID: %d (VP:%d, FP:%d)", program.id, program.vp_id, program.fp_id);
	//wxDialog d_editor(this, wxID_ANY, title, wxDefaultPosition, wxSize(800,500),
	//	wxDEFAULT_DIALOG_STYLE | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxRESIZE_BORDER);

	//wxBoxSizer& s_panel = *new wxBoxSizer(wxHORIZONTAL);
	//wxStaticBoxSizer& s_vp_box = *new wxStaticBoxSizer(wxHORIZONTAL, &d_editor, "Vertex Program");
	//wxStaticBoxSizer& s_fp_box = *new wxStaticBoxSizer(wxHORIZONTAL, &d_editor, "Fragment Program");
	//wxTextCtrl* t_vp_edit  = new wxTextCtrl(&d_editor, wxID_ANY, program.vp_shader, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	//wxTextCtrl* t_fp_edit  = new wxTextCtrl(&d_editor, wxID_ANY, program.fp_shader, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	//t_vp_edit->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	//t_fp_edit->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	//s_vp_box.Add(t_vp_edit, 1, wxEXPAND);
	//s_fp_box.Add(t_fp_edit, 1, wxEXPAND);
	//s_panel.Add(&s_vp_box, 1, wxEXPAND);
	//s_panel.Add(&s_fp_box, 1, wxEXPAND);
	//d_editor.SetSizer(&s_panel);

	//// Show editor and open Save Dialog when closing
	//if (d_editor.ShowModal())
	//{
	//	wxMessageDialog d_save(&d_editor, "Save changes and compile shaders?", title, wxYES_NO|wxCENTRE);
	//	if(d_save.ShowModal() == wxID_YES)
	//	{
	//		program.modified = true;
	//		program.vp_shader = t_vp_edit->GetValue();
	//		program.fp_shader = t_fp_edit->GetValue();
	//	}
	//}
	//UpdateInformation();
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
	if((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) == RSX_METHOD_OLD_JUMP_CMD)
	{
		u32 jumpAddr = cmd & RSX_METHOD_OLD_JUMP_OFFSET_MASK;
		DISASM("JUMP: %08x -> %08x", currentAddr, ioAddr+jumpAddr);
	}
	else if((cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD)
	{
		u32 jumpAddr = cmd & RSX_METHOD_NEW_JUMP_OFFSET_MASK;
		DISASM("JUMP: %08x -> %08x", currentAddr, ioAddr + jumpAddr);
	}
	else if((cmd & RSX_METHOD_CALL_CMD_MASK) == RSX_METHOD_CALL_CMD)
	{
		u32 callAddr = cmd & RSX_METHOD_CALL_OFFSET_MASK;
		DISASM("CALL: %08x -> %08x", currentAddr, ioAddr+callAddr);
	}
	if(cmd == RSX_METHOD_RETURN_CMD)
	{
		DISASM("RETURN");
	}

	if(cmd == 0)
	{
		DISASM("Null cmd");
	}
	else if ((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) != RSX_METHOD_OLD_JUMP_CMD
		&& (cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) != RSX_METHOD_NEW_JUMP_CMD
		&& (cmd & RSX_METHOD_CALL_CMD_MASK) != RSX_METHOD_CALL_CMD
		&& cmd != RSX_METHOD_RETURN_CMD)
	{
		auto args = vm::ps3::ptr<u32>::make(currentAddr + 4);

		u32 index = 0;
		switch((cmd & 0x3ffff) >> 2)
		{
		case 0x3fead:
			DISASM("Flip and change current buffer: %d", (u32)args[0]);
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

		case NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE:
			DISASM(args[0] ? "Depth bounds test: Enable" : "Depth bounds test: Disable");
		break;
		default:
		{
			std::string str = rsx::get_pretty_printing_function((cmd & 0x3ffff) >> 2)((u32)args[0]);
			DISASM("%s", str.c_str());
		}
		}

		if((cmd & RSX_METHOD_NON_INCREMENT_CMD_MASK) == RSX_METHOD_NON_INCREMENT_CMD)
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
