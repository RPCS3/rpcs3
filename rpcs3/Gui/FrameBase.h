#pragma once
#include "rpcs3/Ini.h"
#include "Emu/state.h"

class FrameBase : public wxFrame
{
protected:
	bool m_is_skip_resize;

	FrameBase(
		wxWindow* parent,
		wxWindowID id = wxID_ANY,
		const wxString& framename = "UnknownFrame",
		const std::string& ininame = "",
		wxSize defsize = wxDefaultSize,
		wxPoint defposition = wxDefaultPosition,
		long style = wxDEFAULT_FRAME_STYLE,
		bool is_skip_resize = false)
		: wxFrame(parent, id, framename, defposition, defsize, style)
		, m_is_skip_resize(is_skip_resize)
	{
		LoadInfo();

		Bind(wxEVT_CLOSE_WINDOW, &FrameBase::OnClose, this);
		Bind(wxEVT_MOVE, &FrameBase::OnMove, this);
		Bind(wxEVT_SIZE, &FrameBase::OnResize, this);
	}

	~FrameBase()
	{
	}

	void SetSizerAndFit(wxSizer *sizer, bool deleteOld = true, bool loadinfo = true)
	{
		wxFrame::SetSizerAndFit(sizer, deleteOld);
		if(loadinfo) LoadInfo();
	}

	void LoadInfo()
	{
		SetSize(wxSize(rpcs3::config.gui.size.width.value(), rpcs3::config.gui.size.height.value()));
		SetPosition(wxPoint(rpcs3::config.gui.position.x.value(), rpcs3::config.gui.position.y.value()));
	}

	void OnMove(wxMoveEvent& event)
	{
		rpcs3::config.gui.position.x = GetPosition().x;
		rpcs3::config.gui.position.y = GetPosition().y;
		event.Skip();
	}

	void OnResize(wxSizeEvent& event)
	{
		rpcs3::config.gui.size.width = GetSize().GetWidth();
		rpcs3::config.gui.size.height = GetSize().GetHeight();
		rpcs3::config.gui.position.x = GetPosition().x;
		rpcs3::config.gui.position.y = GetPosition().y;
		if(m_is_skip_resize) event.Skip();
	}

	void OnClose(wxCloseEvent& event)
	{
		rpcs3::config.save();
		event.Skip();
	}
};
