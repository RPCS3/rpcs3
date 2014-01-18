#pragma once

#include <wx/listctrl.h>

class RSXDebugger : public wxFrame
{
	AppConnector m_app_connector;

	u32 m_addr;

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
	wxPanel* p_buffer_tex;

	uint m_cur_texture;

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
	virtual void OnClickBuffer(wxMouseEvent& event);

	virtual void GoToGet(wxCommandEvent& event);
	virtual void GoToPut(wxCommandEvent& event);

	virtual void UpdateInformation();
	virtual void GetMemory();
	virtual void GetBuffers();
	virtual void GetFlags();
	virtual void GetLightning();
	virtual void GetTexture();
	virtual void GetSettings();

	virtual void SetFlags(wxListEvent& event);
	virtual void OnSelectTexture(wxListEvent& event);

	wxString ParseGCMEnum(u32 value, u32 type);
	wxString DisAsmCommand(u32 cmd, u32 count, u32 currentAddr, u32 ioAddr);
	

	bool RSXReady();
	void SetPC(const uint pc) { m_addr = pc; }
};
