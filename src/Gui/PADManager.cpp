#include "stdafx.h"
#include "stdafx_gui.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "rpcs3.h"
#include "PADManager.h"

PADManager::PADManager(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "PAD Settings")
	, m_button_id(0)
	, m_key_pressed(false)
	, m_emu_paused(false)
{
	if(Emu.IsRunning())
	{
		Emu.Pause();
		m_emu_paused = true;
	}

	wxBoxSizer* s_panel = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_subpanel = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel2 = new wxBoxSizer(wxVERTICAL);

	// Left Analog Stick
	wxStaticBoxSizer* s_round_stick_l = new wxStaticBoxSizer(wxVERTICAL, this, _("Left Analog Stick"));
	wxBoxSizer* s_subpanel_lstick_1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_lstick_2 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_subpanel_lstick_3 = new wxBoxSizer(wxVERTICAL);

	// D-Pad
	wxStaticBoxSizer* s_round_pad_controls = new wxStaticBoxSizer(wxVERTICAL, this, _("D-Pad"));
	wxBoxSizer* s_subpanel_pad_1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_pad_2 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_subpanel_pad_3 = new wxBoxSizer(wxVERTICAL);

	// Left shifts
	wxStaticBoxSizer* s_round_pad_shifts_l = new wxStaticBoxSizer(wxVERTICAL, this, _("Left Shifts"));
	wxStaticBoxSizer* s_round_pad_l1 = new wxStaticBoxSizer(wxVERTICAL, this, _("L1"));
	wxStaticBoxSizer* s_round_pad_l2 = new wxStaticBoxSizer(wxVERTICAL, this, _("L2"));
	wxStaticBoxSizer* s_round_pad_l3 = new wxStaticBoxSizer(wxVERTICAL, this, _("L3"));

	// Start / Select
	wxStaticBoxSizer* s_round_pad_system = new wxStaticBoxSizer(wxVERTICAL, this, _("System"));
	wxStaticBoxSizer* s_round_pad_select = new wxStaticBoxSizer(wxVERTICAL, this, _("Select"));
	wxStaticBoxSizer* s_round_pad_start = new wxStaticBoxSizer(wxVERTICAL, this, _("Start"));

	// Right shifts
	wxStaticBoxSizer* s_round_pad_shifts_r = new wxStaticBoxSizer(wxVERTICAL, this, _("Right Shifts"));
	wxStaticBoxSizer* s_round_pad_r1 = new wxStaticBoxSizer(wxVERTICAL, this, _("R1"));
	wxStaticBoxSizer* s_round_pad_r2 = new wxStaticBoxSizer(wxVERTICAL, this, _("R2"));
	wxStaticBoxSizer* s_round_pad_r3 = new wxStaticBoxSizer(wxVERTICAL, this, _("R3"));

	// Action buttons
	wxStaticBoxSizer* s_round_pad_buttons = new wxStaticBoxSizer(wxVERTICAL, this, _("Buttons"));
	wxStaticBoxSizer* s_round_pad_square = new wxStaticBoxSizer(wxVERTICAL, this, _("Square"));
	wxStaticBoxSizer* s_round_pad_cross = new wxStaticBoxSizer(wxVERTICAL, this, _("Cross"));
	wxStaticBoxSizer* s_round_pad_circle = new wxStaticBoxSizer(wxVERTICAL, this, _("Circle"));
	wxStaticBoxSizer* s_round_pad_triangle = new wxStaticBoxSizer(wxVERTICAL, this, _("Triangle"));
	wxBoxSizer* s_subpanel_buttons_1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_buttons_2 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_subpanel_buttons_3 = new wxBoxSizer(wxVERTICAL);

	// Right Analog Stick
	wxStaticBoxSizer* s_round_stick_r = new wxStaticBoxSizer(wxVERTICAL, this, _("Right Analog Stick"));
	wxBoxSizer* s_subpanel_rstick_1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_rstick_2 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_subpanel_rstick_3 = new wxBoxSizer(wxVERTICAL);

	// Ok / Cancel
	wxBoxSizer* s_b_panel(new wxBoxSizer(wxHORIZONTAL));

	wxBoxSizer* s_reset_panel(new wxBoxSizer(wxHORIZONTAL));

	#define ButtonParameters wxEmptyString, wxDefaultPosition, wxSize(60,-1)
	#define SizerFlags       wxSizerFlags().Border(wxALL, 3).Center()


	// Buttons
	b_up_lstick = new wxButton(this, id_pad_lstick_up, ButtonParameters);
	b_down_lstick = new wxButton(this, id_pad_lstick_down, ButtonParameters);
	b_left_lstick = new wxButton(this, id_pad_lstick_left, ButtonParameters);
	b_right_lstick = new wxButton(this, id_pad_lstick_right, ButtonParameters);

	b_up = new wxButton(this, id_pad_up, ButtonParameters);
	b_down = new wxButton(this, id_pad_down, ButtonParameters);
	b_left = new wxButton(this, id_pad_left, ButtonParameters);
	b_right = new wxButton(this, id_pad_right, ButtonParameters);
	
	b_shift_l1 = new wxButton(this, id_pad_l1, ButtonParameters);
	b_shift_l2 = new wxButton(this, id_pad_l2, ButtonParameters);
	b_shift_l3 = new wxButton(this, id_pad_l3, ButtonParameters);

	b_start = new wxButton(this, id_pad_start, ButtonParameters);
	b_select = new wxButton(this, id_pad_select, ButtonParameters);

	b_shift_r1 = new wxButton(this, id_pad_r1, ButtonParameters);
	b_shift_r2 = new wxButton(this, id_pad_r2, ButtonParameters);
	b_shift_r3 = new wxButton(this, id_pad_r3, ButtonParameters);

	b_square = new wxButton(this, id_pad_square, ButtonParameters);
	b_cross = new wxButton(this, id_pad_cross, ButtonParameters);
	b_circle = new wxButton(this, id_pad_circle, ButtonParameters);
	b_triangle = new wxButton(this, id_pad_triangle, ButtonParameters);

	b_up_rstick = new wxButton(this, id_pad_rstick_up, ButtonParameters);
	b_down_rstick = new wxButton(this, id_pad_rstick_down, ButtonParameters);
	b_left_rstick = new wxButton(this, id_pad_rstick_left, ButtonParameters);
	b_right_rstick = new wxButton(this, id_pad_rstick_right, ButtonParameters);

	b_ok = new wxButton(this, wxID_OK, "OK", wxDefaultPosition, wxSize(60,-1));
	b_cancel = new wxButton(this, wxID_CANCEL, "Cancel", wxDefaultPosition, wxSize(60,-1));
	b_reset = new wxButton(this, id_reset_parameters, "By default");


	// Get button labels from .ini
	UpdateLabel();

	// Add buttons in subpanels
	// LStick
	s_subpanel_lstick_1->Add(b_up_lstick);
	s_subpanel_lstick_2->Add(b_left_lstick);
	s_subpanel_lstick_2->Add(b_right_lstick);
	s_subpanel_lstick_3->Add(b_down_lstick);

	// D-Pad
	s_subpanel_pad_1->Add(b_up);
	s_subpanel_pad_2->Add(b_left);
	s_subpanel_pad_2->Add(b_right);
	s_subpanel_pad_3->Add(b_down);

	// Left shifts
	s_round_pad_l1->Add(b_shift_l1);
	s_round_pad_l2->Add(b_shift_l2);
	s_round_pad_l3->Add(b_shift_l3);

	// Start / Select
	s_round_pad_select->Add(b_select);
	s_round_pad_start->Add(b_start);

	// Right shifts
	s_round_pad_r1->Add(b_shift_r1);
	s_round_pad_r2->Add(b_shift_r2);
	s_round_pad_r3->Add(b_shift_r3);

	// Action buttons
	s_round_pad_square->Add(b_square);
	s_round_pad_cross->Add(b_cross);
	s_round_pad_circle->Add(b_circle);
	s_round_pad_triangle->Add(b_triangle);
	s_subpanel_buttons_1->Add(s_round_pad_triangle);
	s_subpanel_buttons_2->Add(s_round_pad_square);
	s_subpanel_buttons_2->Add(s_round_pad_circle);
	s_subpanel_buttons_3->Add(s_round_pad_cross);

	// RStick
	s_subpanel_rstick_1->Add(b_up_rstick);
	s_subpanel_rstick_2->Add(b_left_rstick);
	s_subpanel_rstick_2->Add(b_right_rstick);
	s_subpanel_rstick_3->Add(b_down_rstick);

	// Ok / Cancel
	s_b_panel->Add(b_ok);
	s_b_panel->Add(b_cancel);

	// Reset parameters
	s_reset_panel->Add(b_reset);

	s_round_stick_l->Add(s_subpanel_lstick_1, SizerFlags);
	s_round_stick_l->Add(s_subpanel_lstick_2, SizerFlags);
	s_round_stick_l->Add(s_subpanel_lstick_3, SizerFlags);

	s_round_pad_controls->Add(s_subpanel_pad_1, SizerFlags);
	s_round_pad_controls->Add(s_subpanel_pad_2, SizerFlags);
	s_round_pad_controls->Add(s_subpanel_pad_3, SizerFlags);

	s_round_pad_shifts_l->Add(s_round_pad_l1, SizerFlags);
	s_round_pad_shifts_l->Add(s_round_pad_l2, SizerFlags);
	s_round_pad_shifts_l->Add(s_round_pad_l3, SizerFlags);

	s_round_pad_system->Add(s_round_pad_select, SizerFlags);
	s_round_pad_system->Add(s_round_pad_start, SizerFlags);

	s_round_pad_shifts_r->Add(s_round_pad_r1, SizerFlags);
	s_round_pad_shifts_r->Add(s_round_pad_r2, SizerFlags);
	s_round_pad_shifts_r->Add(s_round_pad_r3, SizerFlags);

	s_round_pad_buttons->Add(s_subpanel_buttons_1, SizerFlags);
	s_round_pad_buttons->Add(s_subpanel_buttons_2, SizerFlags);
	s_round_pad_buttons->Add(s_subpanel_buttons_3, SizerFlags);

	s_round_stick_r->Add(s_subpanel_rstick_1, SizerFlags);
	s_round_stick_r->Add(s_subpanel_rstick_2, SizerFlags);
	s_round_stick_r->Add(s_subpanel_rstick_3, SizerFlags);

	
	s_subpanel->Add(s_round_stick_l);
	s_subpanel->AddSpacer(50);
	s_subpanel->Add(s_b_panel, SizerFlags);

	s_subpanel2->Add(s_round_pad_controls);
	s_subpanel2->AddSpacer(50);
	s_subpanel2->Add(s_reset_panel, SizerFlags);

	// Add subpanels in general panel
	s_panel->Add(s_subpanel);
	s_panel->Add(s_subpanel2);
	s_panel->Add(s_round_pad_shifts_l);
	s_panel->Add(s_round_pad_system);
	s_panel->Add(s_round_pad_shifts_r);
	s_panel->Add(s_round_pad_buttons);
	s_panel->Add(s_round_stick_r);

	SetSizerAndFit(s_panel);

	// Bind buttons
	wxGetApp().Bind(wxEVT_KEY_UP, &PADManager::OnKeyUp, this);
	wxGetApp().Bind(wxEVT_KEY_DOWN, &PADManager::OnKeyDown, this);
	b_up_lstick   ->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_down_lstick ->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_left_lstick ->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_right_lstick->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);

	b_up   ->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_down ->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_left ->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_right->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	
	b_shift_l1->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_shift_l2->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_shift_l3->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);

	b_start ->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_select->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);

	b_shift_r1->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_shift_r2->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_shift_r3->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);

	b_square  ->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_cross   ->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_circle  ->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_triangle->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);

	b_up_rstick   ->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_down_rstick ->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_left_rstick ->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_right_rstick->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);

	b_ok->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_reset->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
	b_cancel->Bind(wxEVT_BUTTON, &PADManager::OnButtonClicked, this);
}

void PADManager::OnKeyDown(wxKeyEvent &keyEvent)
{
	m_key_pressed = true;

	switch (m_button_id)
	{
	case id_pad_lstick_left: rpcs3::config.io.pad.left_stick_left = keyEvent.GetKeyCode(); break;
	case id_pad_lstick_down: rpcs3::config.io.pad.left_stick_down = keyEvent.GetKeyCode(); break;
	case id_pad_lstick_right: rpcs3::config.io.pad.left_stick_right = keyEvent.GetKeyCode(); break;
	case id_pad_lstick_up: rpcs3::config.io.pad.left_stick_up = keyEvent.GetKeyCode(); break;

	case id_pad_left: rpcs3::config.io.pad.left = keyEvent.GetKeyCode(); break;
	case id_pad_down: rpcs3::config.io.pad.down = keyEvent.GetKeyCode(); break;
	case id_pad_right: rpcs3::config.io.pad.right = keyEvent.GetKeyCode(); break;
	case id_pad_up: rpcs3::config.io.pad.up = keyEvent.GetKeyCode(); break;
	
	case id_pad_l1: rpcs3::config.io.pad.l1 = keyEvent.GetKeyCode(); break;
	case id_pad_l2: rpcs3::config.io.pad.l2 = keyEvent.GetKeyCode(); break;
	case id_pad_l3: rpcs3::config.io.pad.l3 = keyEvent.GetKeyCode(); break;

	case id_pad_start: rpcs3::config.io.pad.start = keyEvent.GetKeyCode(); break;
	case id_pad_select: rpcs3::config.io.pad.select = keyEvent.GetKeyCode(); break;
	
	case id_pad_r1: rpcs3::config.io.pad.r1 = keyEvent.GetKeyCode(); break;
	case id_pad_r2: rpcs3::config.io.pad.r2 = keyEvent.GetKeyCode(); break;
	case id_pad_r3: rpcs3::config.io.pad.r3 = keyEvent.GetKeyCode(); break;

	case id_pad_square: rpcs3::config.io.pad.square = keyEvent.GetKeyCode(); break;
	case id_pad_cross: rpcs3::config.io.pad.cross = keyEvent.GetKeyCode(); break;
	case id_pad_circle: rpcs3::config.io.pad.circle = keyEvent.GetKeyCode(); break;
	case id_pad_triangle: rpcs3::config.io.pad.triangle = keyEvent.GetKeyCode(); break;

	case id_pad_rstick_left: rpcs3::config.io.pad.right_stick_left = keyEvent.GetKeyCode(); break;
	case id_pad_rstick_down: rpcs3::config.io.pad.right_stick_down = keyEvent.GetKeyCode(); break;
	case id_pad_rstick_right: rpcs3::config.io.pad.right_stick_right = keyEvent.GetKeyCode(); break;
	case id_pad_rstick_up: rpcs3::config.io.pad.right_stick_up = keyEvent.GetKeyCode(); break;

	case 0: break;
	default: LOG_ERROR(HLE, "Unknown button ID: %d", m_button_id); break;
	}

	UpdateLabel();
	keyEvent.Skip();
}

void PADManager::OnKeyUp(wxKeyEvent &keyEvent)
{
	SwitchButtons(true);  // enable all buttons
	m_button_id = 0; // reset current button id
	m_key_pressed = false;
	keyEvent.Skip();
}

void PADManager::OnButtonClicked(wxCommandEvent &event)
{
	if (event.GetId() != wxID_OK && event.GetId() != wxID_CANCEL && event.GetId() != id_reset_parameters)
	{
		m_button_id = event.GetId();
		SwitchButtons(false); // disable all buttons, needed for using Space, Enter and other specific buttons
		//RunTimer(3, event.GetId()); // TODO: Currently, timer disabled. Use by later, have some strange problems
		//SwitchButtons(true); // needed, if timer enabled
		UpdateLabel();
	}

	else
	{
		switch (event.GetId())
		{
		case id_reset_parameters: ResetParameters(); UpdateLabel(); break;
		case wxID_OK: rpcs3::config.save(); break;
		case wxID_CANCEL: break;

		default: LOG_ERROR(HLE, "Unknown button ID: %d", event.GetId()); break;
		}
	}

	event.Skip();
}

const wxString PADManager::GetKeyName(const u32 keyCode) 
{
	wxString keyName;
	
	switch(keyCode)
	{
	case WXK_NONE: LOG_ERROR(HLE, "Invalid key code"); keyName = "ERROR!"; break;
	case WXK_BACK: keyName = "BackSpace"; break;
	case WXK_TAB: keyName = "Tab"; break;
	case WXK_RETURN: keyName = "Enter"; break;
	case WXK_ESCAPE: keyName = "Esc"; break;
	case WXK_SPACE: keyName = "Space"; break;
	case WXK_DELETE: keyName = "Delete"; break;
	case WXK_SHIFT: keyName = "Shift"; break;
	case WXK_ALT: keyName = "ALT"; break;
	case WXK_CONTROL: keyName = "CTRL"; break;
	case WXK_PAUSE: keyName = "Pause"; break;
	case WXK_CAPITAL: keyName = "CapsLock"; break;
	case WXK_END: keyName = "End"; break;
	case WXK_HOME: keyName = "Home"; break;
	case WXK_LEFT: keyName = "Left"; break;
	case WXK_UP: keyName = "Up"; break;
	case WXK_RIGHT: keyName = "Right"; break;
	case WXK_DOWN: keyName = "Down"; break;
	case WXK_SELECT: keyName = "Select"; break;
	case WXK_PRINT: keyName = "Print"; break;
	case WXK_SNAPSHOT: keyName = "Snapshot"; break;
	case WXK_INSERT: keyName = "Insert"; break;	
	case WXK_NUMPAD0: keyName = "Num0"; break;	
	case WXK_NUMPAD1: keyName = "Num1"; break;	
	case WXK_NUMPAD2: keyName = "Num2"; break;	
	case WXK_NUMPAD3: keyName = "Num3"; break;	
	case WXK_NUMPAD4: keyName = "Num4"; break;	
	case WXK_NUMPAD5: keyName = "Num5"; break;	
	case WXK_NUMPAD6: keyName = "Num6"; break;
	case WXK_NUMPAD7: keyName = "Num7"; break;
	case WXK_NUMPAD8: keyName = "Num8"; break;
	case WXK_NUMPAD9: keyName = "Num9"; break;
	case WXK_F1: keyName = "F1"; break;
	case WXK_F2: keyName = "F2"; break;
	case WXK_F3: keyName = "F3"; break;
	case WXK_F4: keyName = "F4"; break;
	case WXK_F5: keyName = "F5"; break;
	case WXK_F6: keyName = "F6"; break;
	case WXK_F7: keyName = "F7"; break;
	case WXK_F8: keyName = "F8"; break;
	case WXK_F9: keyName = "F9"; break;
	case WXK_F10: keyName = "F10"; break;
	case WXK_F11: keyName = "F11"; break;
	case WXK_F12: keyName = "F12"; break;
	case WXK_NUMLOCK: keyName = "NumLock"; break;
	case WXK_SCROLL: keyName = "ScrollLock"; break;
	case WXK_PAGEUP: keyName = "PgUp"; break;
	case WXK_PAGEDOWN: keyName = "PgDn"; break;
	case WXK_NUMPAD_SPACE: keyName = "NumSpace"; break;
	case WXK_NUMPAD_TAB: keyName = "NumTab"; break;
	case WXK_NUMPAD_ENTER: keyName = "NumEnter"; break;
	case WXK_NUMPAD_HOME: keyName = "NumHome"; break;
	case WXK_NUMPAD_LEFT: keyName = "NumLeft"; break;
	case WXK_NUMPAD_UP: keyName = "NumUp"; break;
	case WXK_NUMPAD_RIGHT: keyName = "NumRight"; break;
	case WXK_NUMPAD_DOWN: keyName = "NumDown"; break;
	case WXK_NUMPAD_PAGEUP: keyName = "NumPgUp"; break;
	case WXK_NUMPAD_PAGEDOWN: keyName = "NumPgDn"; break;
	case WXK_NUMPAD_END: keyName = "NumEnd"; break;
	case WXK_NUMPAD_BEGIN: keyName = "NumHome"; break;
	case WXK_NUMPAD_INSERT: keyName = "NumIns"; break;
	case WXK_NUMPAD_DELETE: keyName = "NumDel"; break;

	default: keyName = static_cast<char>(keyCode); break;
	}

	return keyName;
}

void PADManager::UpdateLabel()
{
	// Get button labels from .ini
	b_up_lstick->SetLabel(GetKeyName(rpcs3::config.io.pad.left_stick_up.value()));
	b_down_lstick->SetLabel(GetKeyName(rpcs3::config.io.pad.left_stick_down.value()));
	b_left_lstick->SetLabel(GetKeyName(rpcs3::config.io.pad.left_stick_left.value()));
	b_right_lstick->SetLabel(GetKeyName(rpcs3::config.io.pad.left_stick_right.value()));

	b_up->SetLabel(GetKeyName(rpcs3::config.io.pad.up.value()));
	b_down->SetLabel(GetKeyName(rpcs3::config.io.pad.down.value()));
	b_left->SetLabel(GetKeyName(rpcs3::config.io.pad.left.value()));
	b_right->SetLabel(GetKeyName(rpcs3::config.io.pad.right.value()));
	
	b_shift_l1->SetLabel(GetKeyName(rpcs3::config.io.pad.l1.value()));
	b_shift_l2->SetLabel(GetKeyName(rpcs3::config.io.pad.l2.value()));
	b_shift_l3->SetLabel(GetKeyName(rpcs3::config.io.pad.l3.value()));

	b_start->SetLabel(GetKeyName(rpcs3::config.io.pad.start.value()));
	b_select->SetLabel(GetKeyName(rpcs3::config.io.pad.select.value()));

	b_shift_r1->SetLabel(GetKeyName(rpcs3::config.io.pad.r1.value()));
	b_shift_r2->SetLabel(GetKeyName(rpcs3::config.io.pad.r2.value()));
	b_shift_r3->SetLabel(GetKeyName(rpcs3::config.io.pad.r3.value()));

	b_square->SetLabel(GetKeyName(rpcs3::config.io.pad.square.value()));
	b_cross->SetLabel(GetKeyName(rpcs3::config.io.pad.cross.value()));
	b_circle->SetLabel(GetKeyName(rpcs3::config.io.pad.circle.value()));
	b_triangle->SetLabel(GetKeyName(rpcs3::config.io.pad.triangle.value()));

	b_up_rstick->SetLabel(GetKeyName(rpcs3::config.io.pad.right_stick_up.value()));
	b_down_rstick->SetLabel(GetKeyName(rpcs3::config.io.pad.right_stick_down.value()));
	b_left_rstick->SetLabel(GetKeyName(rpcs3::config.io.pad.right_stick_left.value()));
	b_right_rstick->SetLabel(GetKeyName(rpcs3::config.io.pad.right_stick_right.value()));
}

void PADManager::ResetParameters()
{
	rpcs3::config.io.pad.left_stick_up = 315;
	rpcs3::config.io.pad.left_stick_down = 317;
	rpcs3::config.io.pad.left_stick_left = 314;
	rpcs3::config.io.pad.left_stick_right = 316;

	rpcs3::config.io.pad.up = static_cast<int>('W');
	rpcs3::config.io.pad.down = static_cast<int>('S');
	rpcs3::config.io.pad.left = static_cast<int>('A');
	rpcs3::config.io.pad.right = static_cast<int>('D');
	
	rpcs3::config.io.pad.l1 = static_cast<int>('1');
	rpcs3::config.io.pad.l2 = static_cast<int>('Q');
	rpcs3::config.io.pad.l3 = static_cast<int>('Z');

	rpcs3::config.io.pad.start = 13;
	rpcs3::config.io.pad.select = 32;

	rpcs3::config.io.pad.r1 = static_cast<int>('3');
	rpcs3::config.io.pad.r2 = static_cast<int>('E');
	rpcs3::config.io.pad.r3 = static_cast<int>('C');

	rpcs3::config.io.pad.square = static_cast<int>('J');
	rpcs3::config.io.pad.cross = static_cast<int>('K');
	rpcs3::config.io.pad.circle = static_cast<int>('L');
	rpcs3::config.io.pad.triangle = static_cast<int>('I');

	rpcs3::config.io.pad.right_stick_up = 366;
	rpcs3::config.io.pad.right_stick_down = 367;
	rpcs3::config.io.pad.right_stick_left = 313;
	rpcs3::config.io.pad.right_stick_right = 312;
}

void PADManager::UpdateTimerLabel(const u32 id)
{
	switch (id)
	{
	case id_pad_lstick_left: b_left_lstick->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_lstick_down: b_down_lstick->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_lstick_right: b_right_lstick->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_lstick_up: b_up_lstick->SetLabel(static_cast<char>(m_seconds + 47)); break;

	case id_pad_left: b_left->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_down: b_down->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_right: b_right->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_up: b_up->SetLabel(static_cast<char>(m_seconds + 47)); break;
	
	case id_pad_l1: b_shift_l1->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_l2: b_shift_l2->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_l3: b_shift_l3->SetLabel(static_cast<char>(m_seconds + 47)); break;

	case id_pad_start: b_start->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_select: b_select->SetLabel(static_cast<char>(m_seconds + 47)); break;
	
	case id_pad_r1: b_shift_r1->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_r2: b_shift_r2->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_r3: b_shift_r3->SetLabel(static_cast<char>(m_seconds + 47)); break;

	case id_pad_square: b_square->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_cross: b_cross->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_circle: b_circle->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_triangle: b_triangle->SetLabel(static_cast<char>(m_seconds + 47)); break;

	case id_pad_rstick_left: b_left_rstick->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_rstick_down: b_down_rstick->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_rstick_right: b_right_rstick->SetLabel(static_cast<char>(m_seconds + 47)); break;
	case id_pad_rstick_up: b_up_rstick->SetLabel(static_cast<char>(m_seconds + 47)); break;
	
	default: LOG_ERROR(HLE, "Unknown button ID: %d", id); break;
	}
}

void PADManager::SwitchButtons(const bool IsEnabled)
{
	b_up_lstick->Enable(IsEnabled);
	b_down_lstick->Enable(IsEnabled);
	b_left_lstick->Enable(IsEnabled);
	b_right_lstick->Enable(IsEnabled);

	b_up->Enable(IsEnabled);
	b_down->Enable(IsEnabled);
	b_left->Enable(IsEnabled);
	b_right->Enable(IsEnabled);
	
	b_shift_l1->Enable(IsEnabled);
	b_shift_l2->Enable(IsEnabled);
	b_shift_l3->Enable(IsEnabled);

	b_start->Enable(IsEnabled);
	b_select->Enable(IsEnabled);

	b_shift_r1->Enable(IsEnabled);
	b_shift_r2->Enable(IsEnabled);
	b_shift_r3->Enable(IsEnabled);

	b_square->Enable(IsEnabled);
	b_cross->Enable(IsEnabled);
	b_circle->Enable(IsEnabled);
	b_triangle->Enable(IsEnabled);

	b_up_rstick->Enable(IsEnabled);
	b_down_rstick->Enable(IsEnabled);
	b_left_rstick->Enable(IsEnabled);
	b_right_rstick->Enable(IsEnabled);

	b_ok->Enable(IsEnabled);
	b_cancel->Enable(IsEnabled);
	b_reset->Enable(IsEnabled);
}

void PADManager::RunTimer(const u32 seconds, const u32 id)		
{
	m_seconds = seconds;
	clock_t t1, t2;
	t1 = t2 = clock() / CLOCKS_PER_SEC;
	while (m_seconds) 
	{
		if (t1 / CLOCKS_PER_SEC + 1 <= (t2 = clock()) / CLOCKS_PER_SEC) 
		{
			UpdateTimerLabel(id);
			m_seconds--;
			t1 = t2;
		}

		if(m_key_pressed)
		{
			m_seconds = 0;
			break;
		}
	}
}
