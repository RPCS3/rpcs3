#pragma once

class InstructionEditorDialog : public wxDialog
{
	u32 pc;
	CPUDisAsm* disasm;
	wxTextCtrl* t2_instr;
	wxStaticText* t3_preview;

public:
	std::weak_ptr<cpu_thread> cpu;

public:
	InstructionEditorDialog(wxPanel *parent, u32 _pc, const std::shared_ptr<cpu_thread>& _cpu, CPUDisAsm* _disasm);

	void updatePreview(wxCommandEvent& event);
};
