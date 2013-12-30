#pragma once

#include <wx/listctrl.h>

class RSXDebugger : public wxFrame
{
	u32 m_addr;
	u32 m_colcount;
	u32 m_rowcount;

	u32 m_buffers_width;
	u32 m_buffers_height;
	u32 m_panel_width;
	u32 m_panel_height;
	u32 m_text_width;
	u32 m_text_height;

	wxTextCtrl* t_addr;

	u32 m_item_count;
	wxListView* m_list_commands;
	wxListView* m_list_flags;
	wxListView* m_list_lightning;
	wxListView* m_list_texture;
	wxListView* m_list_settings;

	wxPanel* p_buffer_colorA;
	wxPanel* p_buffer_colorB;
	wxPanel* p_buffer_colorC;
	wxPanel* p_buffer_colorD;
	wxPanel* p_buffer_depth;
	wxPanel* p_buffer_stencil;
	wxPanel* p_buffer_text;

public:
	bool exit;
	RSXDebugger(wxWindow* parent);
	~RSXDebugger()
	{
		exit = true;
	}

	virtual void OnKeyDown(wxKeyEvent& event);
	virtual void OnChangeToolsAddr(wxCommandEvent& event);
	virtual void OnScrollMemory(wxMouseEvent& event);

	virtual void GoToGet(wxCommandEvent& event);
	virtual void GoToPut(wxCommandEvent& event);

	virtual void UpdateInformation();
	virtual void ShowMemory();
	virtual void ShowBuffers();
	virtual void ShowFlags();

	virtual void ModifyFlags(wxListEvent& event);

	bool RSXReady();
	void SetPC(const uint pc) { m_addr = pc; }
};
