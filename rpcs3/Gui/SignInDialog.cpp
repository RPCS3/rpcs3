#include "stdafx_gui.h"
#include "Emu/Memory/Memory.h"

#include "Emu/SysCalls/Modules/cellSysutil.h"
#include "SignInDialog.h"

void SignInDialogFrame::Create()
{
	wxWindow* parent = nullptr; // TODO: align the window better

	m_dialog = std::make_unique<wxDialog>(parent, wxID_ANY, "Sign in", wxDefaultPosition, wxDefaultSize);
	static const u32 width = 300;
	static const u32 height = 70;

	wxNotebook* nb_config = new wxNotebook(m_dialog.get(), wxID_ANY, wxPoint(2, 2), wxSize(width, height));
	wxPanel* p_esn = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_psn = new wxPanel(nb_config, wxID_ANY);

	wxButton* b_signin = new wxButton(p_esn, wxID_OK, "Fake sign in");

	nb_config->AddPage(p_esn, wxT("ESN"));
	nb_config->AddPage(p_psn, wxT("PSN"));

	wxBoxSizer* s_subpanel_esn = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_psn = new wxBoxSizer(wxVERTICAL);

	wxStaticText* esn_unimplemented = new wxStaticText(p_esn, wxID_ANY, "ESN support is not ready yet.");
	wxStaticText* psn_unimplemented = new wxStaticText(p_psn, wxID_ANY, "PSN support is not yet implemented.");

	// ESN
	s_subpanel_esn->Add(esn_unimplemented, wxSizerFlags().Centre());
	s_subpanel_esn->Add(b_signin, wxSizerFlags().Centre());

	// PSN
	s_subpanel_psn->Add(psn_unimplemented, wxSizerFlags().Centre());

	m_dialog->SetSizerAndFit(s_subpanel_esn, false);
	m_dialog->SetSizerAndFit(s_subpanel_psn, false);

	m_dialog->SetSize(width + 18, height + 42);

	m_dialog->Centre(wxBOTH);
	m_dialog->Show();
	m_dialog->Enable();
	m_dialog->ShowModal();

	m_dialog->Bind(wxEVT_BUTTON, [&](wxCommandEvent& event)
	{
		sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED, 0);
		this->m_dialog->Hide();
		this->Close();
	});

	m_dialog->Bind(wxEVT_CLOSE_WINDOW, [&](wxCloseEvent& event)
	{
		this->m_dialog->Hide();
		this->Close();
	});
}

void SignInDialogFrame::Destroy()
{
	m_dialog.reset();
}