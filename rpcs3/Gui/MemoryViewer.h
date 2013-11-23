#pragma once

#include <wx/listctrl.h>

class MemoryViewerPanel : public wxFrame
{
	//static const uint LINE_COUNT = 50;
	//static const uint COL_COUNT  = 17;

	u32 m_addr;
	u32 m_colcount;
	u32 m_rowcount;
	//wxListView* hex_wind;

	wxTextCtrl* t_addr;
	wxSpinCtrl* sc_bytes;

	wxSpinCtrl* sc_img_size_x;
	wxSpinCtrl* sc_img_size_y;
	wxComboBox* cbox_img_mode;

	wxTextCtrl* t_mem_addr;
	wxTextCtrl* t_mem_hex;
	wxTextCtrl* t_mem_ascii;

public:
	bool exit;
	MemoryViewerPanel(wxWindow* parent);
	~MemoryViewerPanel()
	{
		exit = true;
	}

	//virtual void OnResize(wxSizeEvent& event);
	virtual void OnChangeToolsAddr(wxCommandEvent& event);
	virtual void OnChangeToolsBytes(wxCommandEvent& event);
	virtual void OnScrollMemory(wxMouseEvent& event);

	virtual void Next(wxCommandEvent& event);
	virtual void Prev(wxCommandEvent& event);
	virtual void fNext(wxCommandEvent& event);
	virtual void fPrev(wxCommandEvent& event);

	virtual void ShowMemory();

	void SetPC(const uint pc) { m_addr = pc; }
};