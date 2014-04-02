#pragma once

class FrameBase : public wxFrame
{
protected:
	IniEntry<WindowInfo> m_ini;
	WindowInfo m_default_info;
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
		, m_default_info(defsize, defposition)
		, m_is_skip_resize(is_skip_resize)
	{
		m_ini.Init(ininame.empty() ? fmt::ToUTF8(framename) : ininame, "GuiSettings");
		LoadInfo();

		Connect(GetId(), wxEVT_CLOSE_WINDOW, wxCloseEventHandler(FrameBase::OnClose));
		Connect(GetId(), wxEVT_MOVE, wxMoveEventHandler(FrameBase::OnMove));
		Connect(wxEVT_SIZE, wxSizeEventHandler(FrameBase::OnResize));
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
		const WindowInfo& info = m_ini.LoadValue(m_default_info);
		SetSize(info.size);
		SetPosition(info.position);
	}

	void OnMove(wxMoveEvent& event)
	{
		m_ini.SetValue(WindowInfo(m_ini.GetValue().size, GetPosition()));
		event.Skip();
	}

	void OnResize(wxSizeEvent& event)
	{
		m_ini.SetValue(WindowInfo(GetSize(), m_ini.GetValue().position));
		if(m_is_skip_resize) event.Skip();
	}

	void OnClose(wxCloseEvent& event)
	{
		m_ini.Save();
		event.Skip();
	}
};
