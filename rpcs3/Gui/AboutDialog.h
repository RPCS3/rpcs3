class AboutDialog
	: public wxDialog
{
	enum
	{
		b_id_website,
		b_id_forum
	};

public:
	AboutDialog(wxWindow *parent);

	void OpenWebsite(wxCommandEvent& WXUNUSED(event));
	void OpenForum(wxCommandEvent& WXUNUSED(event));
};

AboutDialog::AboutDialog(wxWindow *parent)
	: wxDialog(parent, wxID_ANY, "About RPCS3", wxDefaultPosition)
{
	wxBoxSizer* s_panel(new wxBoxSizer(wxVERTICAL));

	//Logo
	wxPanel* s_panel_logo(new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(512,92)));
	s_panel_logo->SetBackgroundColour(wxColor(100,100,100));
	
	wxStaticText* t_name = new wxStaticText(this, wxID_ANY, "RPCS3");
	t_name->SetFont(wxFont(28, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	t_name->SetBackgroundColour(wxColor(100,100,100));
	t_name->SetForegroundColour(wxColor(255,255,255));
	t_name->SetPosition(wxPoint(10,6));

	wxStaticText* t_descr = new wxStaticText(this, wxID_ANY, "An early but promising PS3 emulator and debugger.");
	t_descr->SetBackgroundColour(wxColor(100,100,100));
	t_descr->SetForegroundColour(wxColor(255,255,255));
	t_descr->SetPosition(wxPoint(12,50));

#ifdef _DEBUG
	wxStaticText* t_version = new wxStaticText(this, wxID_ANY, wxString::Format("Version: " _PRGNAME_ " git-" RPCS3_GIT_VERSION));
#else
	wxStaticText* t_version = new wxStaticText(this, wxID_ANY, wxString::Format("Version: " _PRGNAME_ " " _PRGVER_));
#endif
	t_version->SetBackgroundColour(wxColor(100,100,100));
	t_version->SetForegroundColour(wxColor(200,200,200));
	t_version->SetPosition(wxPoint(12,66));

	//Credits
	wxBoxSizer* s_panel_credits(new wxBoxSizer(wxHORIZONTAL));
	wxStaticText* t_section1 = new wxStaticText(this, wxID_ANY, "\nDevelopers:\n\nDH\nAlexAltea\nHykem\nOil", wxDefaultPosition, wxSize(156,160));
	wxStaticText* t_section2 = new wxStaticText(this, wxID_ANY, "\nThanks:\n\nBlackDaemon", wxDefaultPosition, wxSize(156,160));

	s_panel_credits->AddSpacer(12);
	s_panel_credits->Add(t_section1);
	s_panel_credits->AddSpacer(8);
	s_panel_credits->Add(t_section2);
	s_panel_credits->AddSpacer(8);
	s_panel_credits->Add(t_section3);
	s_panel_credits->AddSpacer(12);
	
	//Buttons
	wxBoxSizer* s_panel_buttons(new wxBoxSizer(wxHORIZONTAL));
	wxButton* b_website = new wxButton(this, b_id_website, "Website");
	wxButton* b_forum = new wxButton(this, b_id_forum, "Forum");
	Connect(b_id_website, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AboutDialog::OpenWebsite));
	Connect(b_id_forum, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AboutDialog::OpenForum));
	b_website->Disable();

	s_panel_buttons->AddSpacer(12);
	s_panel_buttons->Add(new wxButton(this, wxID_OK), wxLEFT, 0, 5);
	s_panel_buttons->AddSpacer(218);
	s_panel_buttons->Add(b_website, wxLEFT, 0, 5);
	s_panel_buttons->AddSpacer(5);
	s_panel_buttons->Add(b_forum, wxLEFT, 0, 5);
	s_panel_buttons->AddSpacer(12);

	//Panels
	s_panel->Add(s_panel_logo);
	s_panel->Add(s_panel_credits);
	s_panel->Add(s_panel_buttons);
	s_panel->AddSpacer(12);

	SetSizerAndFit(s_panel);
}

void AboutDialog::OpenWebsite(wxCommandEvent& WXUNUSED(event))
{
	wxLaunchDefaultBrowser("http://www.emunewz.net/forum/forumdisplay.php?fid=162");
}

void AboutDialog::OpenForum(wxCommandEvent& WXUNUSED(event))
{
	wxLaunchDefaultBrowser("http://www.emunewz.net/forum/forumdisplay.php?fid=162");
}
