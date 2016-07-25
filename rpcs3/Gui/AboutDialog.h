#pragma once
#include "rpcs3_version.h"

class AboutDialog : public wxDialog
{
	enum
	{
		b_id_github,
		b_id_website,
		b_id_forum,
	};

public:
	AboutDialog(wxWindow* parent)
		: wxDialog(parent, wxID_ANY, "About RPCS3", wxDefaultPosition)
	{
		wxBoxSizer* s_panel(new wxBoxSizer(wxVERTICAL));

		//Logo
		wxPanel* s_panel_logo(new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(512, 92)));
		s_panel_logo->SetBackgroundColour(wxColor(100, 100, 100));

		wxStaticText* t_name = new wxStaticText(this, wxID_ANY, "RPCS3");
		t_name->SetFont(wxFont(28, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		t_name->SetBackgroundColour(wxColor(100, 100, 100));
		t_name->SetForegroundColour(wxColor(255, 255, 255));
		t_name->SetPosition(wxPoint(10, 6));

		wxStaticText* t_descr = new wxStaticText(this, wxID_ANY, "PS3 emulator and debugger.");
		t_descr->SetBackgroundColour(wxColor(100, 100, 100));
		t_descr->SetForegroundColour(wxColor(255, 255, 255));
		t_descr->SetPosition(wxPoint(12, 50));

		wxStaticText* t_version = new wxStaticText(this, wxID_ANY, "RPCS3 Version: " + rpcs3::version.to_string());
		t_version->SetBackgroundColour(wxColor(100, 100, 100));
		t_version->SetForegroundColour(wxColor(200, 200, 200));
		t_version->SetPosition(wxPoint(12, 66));

		//Credits
		wxBoxSizer* s_panel_credits(new wxBoxSizer(wxHORIZONTAL));
		wxStaticText* t_section1 = new wxStaticText(this, wxID_ANY, "\nDevelopers:\n\nDH\nAlexAltea\nHykem\nOil\nNekotekina\nelisha464\nBigpet\nvlj\n");
		wxStaticText* t_section2 = new wxStaticText(this, wxID_ANY, "\nThanks:\n\nBlackDaemon\nAishou\nkrofna\nxsacha\n");

		s_panel_credits->AddSpacer(12);
		s_panel_credits->Add(t_section1, 5);
		s_panel_credits->AddStretchSpacer();
		s_panel_credits->Add(t_section2, 5);
		s_panel_credits->AddSpacer(12);

		//Buttons
		wxBoxSizer* s_panel_buttons(new wxBoxSizer(wxHORIZONTAL));
		wxButton* b_github = new wxButton(this, b_id_github, "GitHub");
		wxButton* b_website = new wxButton(this, b_id_website, "Website");
		wxButton* b_forum = new wxButton(this, b_id_forum, "Forum");
		Connect(b_id_github, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AboutDialog::OpenWebsite));
		Connect(b_id_website, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AboutDialog::OpenWebsite));
		Connect(b_id_forum, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AboutDialog::OpenWebsite));

		s_panel_buttons->AddSpacer(12);
		s_panel_buttons->Add(b_github, 16, 0, 5);
		s_panel_buttons->AddStretchSpacer();
		s_panel_buttons->Add(b_website, 16, 0, 5);
		s_panel_buttons->AddStretchSpacer();
		s_panel_buttons->Add(b_forum, 16, 0, 5);
		s_panel_buttons->AddStretchSpacer(20);
		s_panel_buttons->Add(new wxButton(this, wxID_OK), 16, 0, 5);
		s_panel_buttons->AddSpacer(12);

		//Panels
		s_panel->Add(s_panel_logo);
		s_panel->Add(s_panel_credits, 0, wxEXPAND);
		s_panel->Add(s_panel_buttons, 0, wxEXPAND);
		s_panel->AddSpacer(12);

		SetSizerAndFit(s_panel);
	}

	void OpenWebsite(wxCommandEvent& event)
	{
		switch (event.GetId())
		{
		case b_id_github: wxLaunchDefaultBrowser("https://github.com/RPCS3"); break;
		case b_id_website: wxLaunchDefaultBrowser("http://rpcs3.net/"); break;
		case b_id_forum: wxLaunchDefaultBrowser("http://www.emunewz.net/forum/forumdisplay.php?fid=162"); break;
		}
	}
};
