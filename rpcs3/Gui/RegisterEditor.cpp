#include "stdafx.h"
#include "stdafx_gui.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "RegisterEditor.h"

RegisterEditorDialog::RegisterEditorDialog(wxPanel *parent, u32 _pc, cpu_thread* _cpu, CPUDisAsm* _disasm)
	: wxDialog(parent, wxID_ANY, "Edit registers")
	, pc(_pc)
	, cpu(_cpu)
	, disasm(_disasm)
{
	wxBoxSizer* s_panel_margin_x = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_panel_margin_y = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* s_panel = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_t1_panel = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_t2_panel = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_t3_panel = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_b_panel = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* t1_text = new wxStaticText(this, wxID_ANY, "Register:     ");
	t1_register = new wxComboBox(this, wxID_ANY);
	wxStaticText* t2_text = new wxStaticText(this, wxID_ANY, "Value (Hex):");
	t2_value = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1));

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

	Bind(wxEVT_COMBOBOX, &RegisterEditorDialog::updateRegister, this);

	switch (cpu->type)
	{
	case cpu_type::ppu:
		for (int i = 0; i<32; i++) t1_register->Append(wxString::Format("GPR[%d]", i));
		for (int i = 0; i<32; i++) t1_register->Append(wxString::Format("FPR[%d]", i));
		for (int i = 0; i<32; i++) t1_register->Append(wxString::Format("VR[%d]", i));
		t1_register->Append("CR");
		t1_register->Append("LR");
		t1_register->Append("CTR");
		//t1_register->Append("XER");
		//t1_register->Append("FPSCR");
		break;

	case cpu_type::spu:
		for (int i = 0; i<128; i++) t1_register->Append(wxString::Format("GPR[%d]", i));
		break;

	default:
		wxMessageBox("Not supported thread.", "Error");
		return;
	}

	SetSizerAndFit(s_panel_margin_x);

	if (ShowModal() == wxID_OK)
	{
		std::string reg = fmt::ToUTF8(t1_register->GetStringSelection());
		std::string value = fmt::ToUTF8(t2_value->GetValue());

		switch (cpu->type)
		{
		case cpu_type::ppu:
		{
			auto& ppu = *static_cast<PPUThread*>(cpu);

			while (value.length() < 32) value = "0" + value;
			std::string::size_type first_brk = reg.find('[');
			try
			{
				if (first_brk != std::string::npos)
				{
					long reg_index = atol(reg.substr(first_brk + 1, reg.length() - first_brk - 2).c_str());
					if (reg.find("GPR") == 0 || reg.find("FPR") == 0)
					{
						unsigned long long reg_value;
						reg_value = std::stoull(value.substr(16, 31), 0, 16);
						if (reg.find("GPR") == 0) ppu.GPR[reg_index] = (u64)reg_value;
						if (reg.find("FPR") == 0) (u64&)ppu.FPR[reg_index] = (u64)reg_value;
						return;
					}
					if (reg.find("VR") == 0)
					{
						unsigned long long reg_value0;
						unsigned long long reg_value1;
						reg_value0 = std::stoull(value.substr(16, 31), 0, 16);
						reg_value1 = std::stoull(value.substr(0, 15), 0, 16);
						ppu.VR[reg_index]._u64[0] = (u64)reg_value0;
						ppu.VR[reg_index]._u64[1] = (u64)reg_value1;
						return;
					}
				}
				if (reg == "LR" || reg == "CTR")
				{
					unsigned long long reg_value;
					reg_value = std::stoull(value.substr(16, 31), 0, 16);
					if (reg == "LR") ppu.LR = (u64)reg_value;
					if (reg == "CTR") ppu.CTR = (u64)reg_value;
					return;
				}
				if (reg == "CR")
				{
					unsigned long long reg_value;
					reg_value = std::stoull(value.substr(24, 31), 0, 16);
					if (reg == "CR") ppu.SetCR((u32)reg_value);
					return;
				}
			}
			catch (std::invalid_argument&)//if any of the stoull conversion fail
			{
				break;
			}

			break;
		}
		case cpu_type::spu:
		{
			auto& spu = *static_cast<SPUThread*>(cpu);

			while (value.length() < 32) value = "0" + value;
			std::string::size_type first_brk = reg.find('[');
			if (first_brk != std::string::npos)
			{
				long reg_index;
				reg_index = atol(reg.substr(first_brk + 1, reg.length() - 2).c_str());
				if (reg.find("GPR") == 0)
				{
					ullong reg_value0;
					ullong reg_value1;
					try
					{
						reg_value0 = std::stoull(value.substr(16, 31), 0, 16);
						reg_value1 = std::stoull(value.substr(0, 15), 0, 16);
					}
					catch (std::invalid_argument& /*e*/)
					{
						break;
					}
					spu.gpr[reg_index]._u64[0] = (u64)reg_value0;
					spu.gpr[reg_index]._u64[1] = (u64)reg_value1;
					return;
				}
			}
			
			break;
		}
		}

		wxMessageBox("This value could not be converted.\nNo changes were made.", "Error");
	}
}

void RegisterEditorDialog::updateRegister(wxCommandEvent& event)
{
	std::string reg = fmt::ToUTF8(t1_register->GetStringSelection());
	std::string str;

	switch (cpu->type)
	{
	case cpu_type::ppu:
	{
		auto& ppu = *static_cast<PPUThread*>(cpu);

		std::size_t first_brk = reg.find('[');
		if (first_brk != -1)
		{
			long reg_index = std::atol(reg.substr(first_brk + 1, reg.length() - first_brk - 2).c_str());
			if (reg.find("GPR") == 0) str = fmt::format("%016llx", ppu.GPR[reg_index]);
			if (reg.find("FPR") == 0) str = fmt::format("%016llx", ppu.FPR[reg_index]);
			if (reg.find("VR") == 0)  str = fmt::format("%016llx%016llx", ppu.VR[reg_index]._u64[1], ppu.VR[reg_index]._u64[0]);
		}
		if (reg == "CR")  str = fmt::format("%08x", ppu.GetCR());
		if (reg == "LR")  str = fmt::format("%016llx", ppu.LR);
		if (reg == "CTR") str = fmt::format("%016llx", ppu.CTR);
		break;
	}
	case cpu_type::spu:
	{
		auto& spu = *static_cast<SPUThread*>(cpu);

		std::string::size_type first_brk = reg.find('[');
		if (first_brk != std::string::npos)
		{
			long reg_index;
			reg_index = atol(reg.substr(first_brk + 1, reg.length() - 2).c_str());
			if (reg.find("GPR") == 0) str = fmt::format("%016llx%016llx", spu.gpr[reg_index]._u64[1], spu.gpr[reg_index]._u64[0]);
		}
		break;
	}
	}

	t2_value->SetValue(fmt::FromUTF8(str));
}
