class InstructionEditorDialog
	: public wxDialog
{
	u64 pc;
	PPC_DisAsm* disasm;
	PPC_Decoder* decoder;
	wxTextCtrl* t2_instr;
	wxStaticText* t3_preview;

public:
	PPCThread* CPU;

public:
	InstructionEditorDialog(wxPanel *parent, u64 _pc, PPCThread* _CPU, PPC_Decoder* _decoder, PPC_DisAsm* _disasm);

	void updatePreview(wxCommandEvent& event);
};

InstructionEditorDialog::InstructionEditorDialog(wxPanel *parent, u64 _pc, PPCThread* _CPU, PPC_Decoder* _decoder, PPC_DisAsm* _disasm)
	: wxDialog(parent, wxID_ANY, "Edit instruction", wxDefaultPosition)
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

	wxStaticText* t1_text = new wxStaticText(this, wxID_ANY, "Address:     ");
	wxStaticText* t1_addr = new wxStaticText(this, wxID_ANY, wxString::Format("%08x",pc));
	wxStaticText* t2_text = new wxStaticText(this, wxID_ANY, "Instruction:");
	t2_instr = new wxTextCtrl(this, wxID_ANY);
	wxStaticText* t3_text = new wxStaticText(this, wxID_ANY, "Preview:     ");
	t3_preview = new wxStaticText(this, wxID_ANY, "");

	s_t1_panel->Add(t1_text);
	s_t1_panel->AddSpacer(8);
	s_t1_panel->Add(t1_addr);

	s_t2_panel->Add(t2_text);
	s_t2_panel->AddSpacer(8);
	s_t2_panel->Add(t2_instr);

	s_t3_panel->Add(t3_text);
	s_t3_panel->AddSpacer(8);
	s_t3_panel->Add(t3_preview);

	s_b_panel->Add(new wxButton(this, wxID_OK), wxLEFT, 0, 5);
	s_b_panel->AddSpacer(5);
	s_b_panel->Add(new wxButton(this, wxID_CANCEL), wxRIGHT, 0, 5);

	s_panel->Add(s_t1_panel);
	s_panel->AddSpacer(8);
	s_panel->Add(s_t3_panel);
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

	this->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(InstructionEditorDialog::updatePreview));
	t2_instr->SetValue(wxString::Format("%08x",	Memory.Read32(CPU->GetOffset() + pc)));

	this->SetSizerAndFit(s_panel_margin_x);

	if(this->ShowModal() == wxID_OK)
	{
		unsigned long opcode;
		if (!t2_instr->GetValue().ToULong(&opcode, 16))
			wxMessageBox("This instruction could not be parsed.\nNo changes were made.","Error");
		else
			Memory.Write32(CPU->GetOffset() + pc, (u32)opcode);
	}
}

void InstructionEditorDialog::updatePreview(wxCommandEvent& event)
{
	unsigned long opcode;
	if (t2_instr->GetValue().ToULong(&opcode, 16))
	{
		decoder->Decode((u32)opcode);
		wxString preview = disasm->last_opcode;
		while (preview[0] != ':') preview.Remove(0,1);
		preview.Remove(0,1);
		t3_preview->SetLabel(preview);
	}
	else
	{
		t3_preview->SetLabel("Could not parse instruction.");
	}
}