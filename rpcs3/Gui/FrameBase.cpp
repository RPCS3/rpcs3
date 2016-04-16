#include "stdafx.h"
#include "stdafx_gui.h"
#include "FrameBase.h"

FrameBase::FrameBase(wxWindow* parent, wxWindowID id, const wxString& frame_name, const std::string& ini_name, wxSize defsize, wxPoint defposition, long style)
	: wxFrame(parent, id, frame_name, defposition, defsize, style)
	, ini_name(ini_name.empty() ? fmt::ToUTF8(frame_name) : ini_name)
{
	LoadInfo();

	Bind(wxEVT_CLOSE_WINDOW, &FrameBase::OnClose, this);
	Bind(wxEVT_MOVE, &FrameBase::OnMove, this);
	Bind(wxEVT_SIZE, &FrameBase::OnResize, this);
}

void FrameBase::SetSizerAndFit(wxSizer* sizer, bool deleteOld, bool loadinfo)
{
	wxFrame::SetSizerAndFit(sizer, deleteOld);
	if (loadinfo) LoadInfo();
}

void FrameBase::LoadInfo()
{
	auto&& cfg = g_gui_cfg[ini_name];

	auto&& size = GetSize();
	std::tie(size.x, size.y) = cfg["size"].as<std::pair<int, int>>(std::make_pair(size.x, size.y));
	SetSize(size);

	auto&& pos = GetPosition();
	std::tie(pos.x, pos.y) = cfg["pos"].as<std::pair<int, int>>(std::make_pair(pos.x, pos.y));
	SetPosition(pos);
}

void FrameBase::OnMove(wxMoveEvent& event)
{
	auto&& cfg = g_gui_cfg[ini_name];

	const auto& pos = GetPosition();
	cfg["pos"] = std::make_pair(pos.x, pos.y);

	event.Skip();
}

void FrameBase::OnResize(wxSizeEvent& event)
{
	auto&& cfg = g_gui_cfg[ini_name];

	const auto& size = GetSize();
	cfg["size"] = std::make_pair(size.x, size.y);

	const auto& pos = GetPosition();
	cfg["pos"] = std::make_pair(pos.x, pos.y);
}

void FrameBase::OnClose(wxCloseEvent& event)
{
	save_gui_cfg();
	event.Skip();
}
