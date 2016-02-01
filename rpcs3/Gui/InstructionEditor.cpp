#include "stdafx.h"
#include "stdafx_gui.h"
#include "Emu/Memory/Memory.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "InstructionEditor.h"

InstructionEditorDialog::InstructionEditorDialog(wxPanel *parent, u32 _pc, cpu_thread* _cpu, CPUDisAsm* _disasm)
	: wxDialog(parent, wxID_ANY, "Edit instruction", wxDefaultPosition)
	, pc(_pc)
	, cpu(_cpu)
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
	wxStaticText* t1_addr = new wxStaticText(this, wxID_ANY, wxString::Format("%08x", pc));
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

	const u32 cpu_offset = cpu->type == cpu_type::spu ? static_cast<SPUThread&>(*cpu).offset : 0;

	this->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(InstructionEditorDialog::updatePreview));
	t2_instr->SetValue(wxString::Format("%08x", vm::ps3::read32(cpu_offset + pc).value()));

	this->SetSizerAndFit(s_panel_margin_x);

	if (this->ShowModal() == wxID_OK)
	{
		ulong opcode;
		if (!t2_instr->GetValue().ToULong(&opcode, 16))
			wxMessageBox("This instruction could not be parsed.\nNo changes were made.", "Error");
		else
			vm::ps3::write32(cpu_offset + pc, (u32)opcode);
	}
}

void InstructionEditorDialog::updatePreview(wxCommandEvent& event)
{
	ulong opcode;
	if (t2_instr->GetValue().ToULong(&opcode, 16))
	{
		if (cpu->type == cpu_type::arm)
		{
			t3_preview->SetLabel("Preview for ARMv7Thread not implemented yet.");
		}
		else
		{
			t3_preview->SetLabel("Preview disabled.");
		}
	}
	else
	{
		t3_preview->SetLabel("Could not parse instruction.");
	}
}
