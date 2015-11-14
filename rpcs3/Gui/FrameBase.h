#pragma once
#include "rpcs3/Ini.h"
#include "Emu/state.h"

class FrameBase : public wxFrame
{
protected:
	bool m_is_skip_resize;
	std::string m_ini_name;

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
		//TODO
		m_ini_name = ininame.empty() ? fmt::ToUTF8(framename) : ininame;

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
		size2i size = rpcs3::config.gui.size.value();
		position2i position = rpcs3::config.gui.position.value();
		SetSize(wxSize(size.width, size.height));
		SetPosition(wxPoint(position.x, position.y));
	}

	void OnMove(wxMoveEvent& event)
	{
		rpcs3::config.gui.position = position2i{ GetPosition().x, GetPosition().y };
		event.Skip();
	}

	void OnResize(wxSizeEvent& event)
	{
		rpcs3::config.gui.size = size2i{ GetSize().GetWidth(), GetSize().GetHeight() };
		rpcs3::config.gui.position = position2i{ GetPosition().x, GetPosition().y };
		if(m_is_skip_resize) event.Skip();
	}

	void OnClose(wxCloseEvent& event)
	{
		rpcs3::config.save();
		event.Skip();
	}
};
