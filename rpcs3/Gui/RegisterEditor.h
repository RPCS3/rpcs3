#pragma once

class RegisterEditorDialog : public wxDialog
{
	u32 pc;
	CPUDisAsm* disasm;
	wxComboBox* t1_register;
	wxTextCtrl* t2_value;
	wxStaticText* t3_preview;

public:
	std::weak_ptr<cpu_thread> cpu;

public:
	RegisterEditorDialog(wxPanel *parent, u32 _pc, const std::shared_ptr<cpu_thread>& _cpu, CPUDisAsm* _disasm);

	void updateRegister(wxCommandEvent& event);
};
