class RegisterEditorDialog
	: public wxDialog
{
	u64 pc;
	PPC_DisAsm* disasm;
	PPC_Decoder* decoder;
	wxComboBox* t1_register;
	wxTextCtrl* t2_value;
	wxStaticText* t3_preview;

public:
	PPCThread* CPU;

public:
	RegisterEditorDialog(wxPanel *parent, u64 _pc, PPCThread* _CPU, PPC_Decoder* _decoder, PPC_DisAsm* _disasm);

	void updateRegister(wxCommandEvent& event);
	void updatePreview(wxCommandEvent& event);
};

RegisterEditorDialog::RegisterEditorDialog(wxPanel *parent, u64 _pc, PPCThread* _CPU, PPC_Decoder* _decoder, PPC_DisAsm* _disasm)
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

	this->Connect(wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler(RegisterEditorDialog::updateRegister));

	if (CPU->GetType() == PPC_THREAD_PPU)
	{
		for (int i=0; i<32; i++) t1_register->Append(wxString::Format("GPR[%d]",i));
		for (int i=0; i<32; i++) t1_register->Append(wxString::Format("FPR[%d]",i));
		for (int i=0; i<32; i++) t1_register->Append(wxString::Format("VPR[%d]",i));
		t1_register->Append("CR");
		t1_register->Append("LR");
		t1_register->Append("CTR");
		t1_register->Append("XER");
		t1_register->Append("FPSCR");
	}
	if (CPU->GetType() == PPC_THREAD_SPU)
	{
		for (int i=0; i<128; i++) t1_register->Append(wxString::Format("GPR[%d]",i));
	}
	if (CPU->GetType() == PPC_THREAD_RAW_SPU)
	{
		wxMessageBox("RawSPU threads not yet supported.","Error");
		return;
	}

	this->SetSizerAndFit(s_panel_margin_x);

	if(this->ShowModal() == wxID_OK)
	{
		wxString reg = t1_register->GetStringSelection();
		wxString value = t2_value->GetValue();
		if (!CPU->WriteRegString(reg,value))
			wxMessageBox("This value could not be converted.\nNo changes were made.","Error");
	}
}

void RegisterEditorDialog::updateRegister(wxCommandEvent& event)
{
	wxString reg = t1_register->GetStringSelection();
	t2_value->SetValue(CPU->ReadRegString(reg));
}