class RegisterEditorDialog : public wxDialog
{
	u64 pc;
	CPUDisAsm* disasm;
	CPUDecoder* decoder;
	wxComboBox* t1_register;
	wxTextCtrl* t2_value;
	wxStaticText* t3_preview;

public:
	CPUThread* CPU;

public:
	RegisterEditorDialog(wxPanel *parent, u64 _pc, CPUThread* _CPU, CPUDecoder* _decoder, CPUDisAsm* _disasm);

	void updateRegister(wxCommandEvent& event);
	void updatePreview(wxCommandEvent& event);
};

RegisterEditorDialog::RegisterEditorDialog(wxPanel *parent, u64 _pc, CPUThread* _CPU, CPUDecoder* _decoder, CPUDisAsm* _disasm)
	: wxDialog(parent, wxID_ANY, "Edit registers", wxDefaultPosition)
	, pc(_pc)
	, CPU(_CPU)
	, decoder(_decoder)
	, disasm(_disasm)
{
	wxBoxSizer* s_panel_margin_x(new wxBoxSizer(wxHORIZONTAL));
	wxBoxSizer* s_panel_margin_y(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* s_panel(new wxBoxSizer(wxVERTICAL));
	wxBoxSizer* s_t1_panel(new wxBoxSizer(wxHORIZONTAL));
	wxBoxSizer* s_t2_panel(new wxBoxSizer(wxHORIZONTAL));
	wxBoxSizer* s_t3_panel(new wxBoxSizer(wxHORIZONTAL));
	wxBoxSizer* s_b_panel(new wxBoxSizer(wxHORIZONTAL));

	wxStaticText* t1_text = new wxStaticText(this, wxID_ANY, "Register:     ");
	t1_register = new wxComboBox(this, wxID_ANY, wxEmptyString);
	wxStaticText* t2_text = new wxStaticText(this, wxID_ANY, "Value (Hex):");
	t2_value = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200,-1));

	s_t1_panel->Add(t1_text);
	s_t1_panel->AddSpacer(8);
	s_t1_panel->Add(t1_register);

	s_t2_panel->Add(t2_text);
	s_t2_panel->AddSpacer(8);
	s_t2_panel->Add(t2_value);

	s_b_panel->Add(new wxButton(this, wxID_OK), wxLEFT, 0, 5);
	s_b_panel->AddSpacer(5);
	s_b_panel->Add(new wxButton(this, wxID_CANCEL), wxLEFT, 0, 5);

	s_panel->Add(s_t1_panel);
	s_panel->AddSpacer(8);
	s_panel->Add(s_t2_panel);
	s_panel->AddSpacer(16);
	s_panel->Add(s_b_panel);

	s_panel_margin_y->AddSpacer(12);
	s_panel_margin_y->Add(s_panel);
	s_panel_margin_y->AddSpacer(12);
	s_panel_margin_x->AddSpacer(12);
	s_panel_margin_x->Add(s_panel_margin_y);
	s_panel_margin_x->AddSpacer(12);

	Connect(wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler(RegisterEditorDialog::updateRegister));

	switch(CPU->GetType())
	{
	case CPU_THREAD_PPU:
		for (int i=0; i<32; i++) t1_register->Append(wxString::Format("GPR[%d]",i));
		for (int i=0; i<32; i++) t1_register->Append(wxString::Format("FPR[%d]",i));
		for (int i=0; i<32; i++) t1_register->Append(wxString::Format("VPR[%d]",i));
		t1_register->Append("CR");
		t1_register->Append("LR");
		t1_register->Append("CTR");
		t1_register->Append("XER");
		t1_register->Append("FPSCR");
	break;

	case CPU_THREAD_SPU:
	case CPU_THREAD_RAW_SPU:
		for (int i=0; i<128; i++) t1_register->Append(wxString::Format("GPR[%d]",i));
	break;

	default:
		wxMessageBox("Not supported thread.", "Error");
	return;
	}

	SetSizerAndFit(s_panel_margin_x);

	if(ShowModal() == wxID_OK)
	{
		std::string reg = fmt::ToUTF8(t1_register->GetStringSelection());
		std::string value = fmt::ToUTF8(t2_value->GetValue());
		if (!CPU->WriteRegString(reg,value))
			wxMessageBox("This value could not be converted.\nNo changes were made.","Error");
	}
}

void RegisterEditorDialog::updateRegister(wxCommandEvent& event)
{
	std::string reg = fmt::ToUTF8(t1_register->GetStringSelection());
	t2_value->SetValue(fmt::FromUTF8(CPU->ReadRegString(reg)));
}
