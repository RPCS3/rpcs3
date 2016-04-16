#pragma once

class RegisterEditorDialog : public wxDialog
{
	u32 pc;
	CPUDisAsm* disasm;
	wxComboBox* t1_register;
	wxTextCtrl* t2_value;
	wxStaticText* t3_preview;

public:
	cpu_thread* cpu;

public:
	RegisterEditorDialog(wxPanel *parent, u32 _pc, cpu_thread* _cpu, CPUDisAsm* _disasm);

	void updateRegister(wxCommandEvent& event);
};
