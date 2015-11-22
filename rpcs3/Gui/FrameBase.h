#pragma once

struct FrameBase : public wxFrame
{
	const std::string ini_name;

protected:
	FrameBase(
		wxWindow* parent,
		wxWindowID id = wxID_ANY,
		const wxString& frame_name = "Unknown frame",
		const std::string& ini_name = {},
		wxSize defsize = wxDefaultSize,
		wxPoint defposition = wxDefaultPosition,
		long style = wxDEFAULT_FRAME_STYLE);

	~FrameBase()
	{
	}

	void SetSizerAndFit(wxSizer* sizer, bool deleteOld = true, bool loadinfo = true);
	void LoadInfo();
	void OnMove(wxMoveEvent& event);
	void OnResize(wxSizeEvent& event);
	void OnClose(wxCloseEvent& event);
};
