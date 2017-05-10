
#include "RegisterEditor.h"

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }

RegisterEditorDialog::RegisterEditorDialog(QWidget *parent, u32 _pc, const std::shared_ptr<cpu_thread>& _cpu, CPUDisAsm* _disasm)
	: QDialog(parent)
	, pc(_pc)
	, cpu(_cpu)
	, disasm(_disasm)
{
	setWindowTitle(tr("Edit registers"));
	QHBoxLayout* hbox_panel_margin_x = new QHBoxLayout();
	QVBoxLayout* vbox_panel_margin_y = new QVBoxLayout();

	QVBoxLayout* vbox_panel = new QVBoxLayout();
	QHBoxLayout* hbox_t1_panel = new QHBoxLayout();
	QHBoxLayout* hbox_t2_panel = new QHBoxLayout();
	QHBoxLayout* hbox_t3_panel = new QHBoxLayout();
	QHBoxLayout* hbox_b_panel = new QHBoxLayout();

	QLabel* t1_text = new QLabel(tr("Register:     "), this);
	QLabel* t2_text = new QLabel(tr("Value (Hex):"), this);

	QPushButton* b_ok = new QPushButton(tr("&Ok"));
	QPushButton* b_cancel = new QPushButton(tr("&Cancel"));
	
	t1_register = new QComboBox(this);
	t2_value = new QLineEdit(this);
	t2_value->setFixedWidth(200);

	hbox_t1_panel->addWidget(t1_text);
	hbox_t1_panel->addWidget(t1_register);

	hbox_t2_panel->addWidget(t2_text);
	hbox_t2_panel->addWidget(t2_value);

	hbox_b_panel->addWidget(b_ok);
	hbox_b_panel->addWidget(b_cancel);

	vbox_panel->addLayout(hbox_t1_panel);
	vbox_panel->addLayout(hbox_t2_panel);
	vbox_panel->addLayout(hbox_b_panel);

	vbox_panel_margin_y->addLayout(vbox_panel);
	hbox_panel_margin_x->addLayout(vbox_panel_margin_y);

	connect(t1_register, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &RegisterEditorDialog::updateRegister);

	switch (g_system)
	{
	case system_type::ps3:
	{
		if (_cpu->id_type() == 1)
		{
			for (int i = 0; i < 32; i++) t1_register->addItem(qstr(fmt::format("GPR[%d]", i)));
			for (int i = 0; i < 32; i++) t1_register->addItem(qstr(fmt::format("FPR[%d]", i)));
			for (int i = 0; i < 32; i++) t1_register->addItem(qstr(fmt::format("VR[%d]", i)));
			t1_register->addItem("CR");
			t1_register->addItem("LR");
			t1_register->addItem("CTR");
			//t1_register->addItem("XER");
			//t1_register->addItem("FPSCR");
		}
		else
		{
			for (int i = 0; i < 128; i++) t1_register->addItem(qstr(fmt::format("GPR[%d]", i)));
		}

		break;
	}

	default:
		QMessageBox::critical(this, tr("Error"), tr("Not supported thread."));
		return;
	}

	setLayout(hbox_panel_margin_x);

	if (isModal())
	{
		const auto cpu = _cpu.get();

		std::string reg = t1_register->itemData(t1_register->currentIndex()).toString().toUtf8().toStdString();
		std::string value = t2_value->text().toUtf8().toStdString();

		if (g_system == system_type::ps3 && cpu->id_type() == 1)
		{
			auto& ppu = *static_cast<ppu_thread*>(cpu);

			while (value.length() < 32) value = "0" + value;
			const auto first_brk = reg.find('[');
			try
			{
				if (first_brk != -1)
				{
					const long reg_index = std::atol(reg.substr(first_brk + 1, reg.length() - first_brk - 2).c_str());
					if (reg.find("GPR") == 0 || reg.find("FPR") == 0)
					{
						const ullong reg_value = std::stoull(value.substr(16, 31), 0, 16);
						if (reg.find("GPR") == 0) ppu.gpr[reg_index] = (u64)reg_value;
						if (reg.find("FPR") == 0) (u64&)ppu.fpr[reg_index] = (u64)reg_value;
						return;
					}
					if (reg.find("VR") == 0)
					{
						const ullong reg_value0 = std::stoull(value.substr(16, 31), 0, 16);
						const ullong reg_value1 = std::stoull(value.substr(0, 15), 0, 16);
						ppu.vr[reg_index]._u64[0] = (u64)reg_value0;
						ppu.vr[reg_index]._u64[1] = (u64)reg_value1;
						return;
					}
				}
				if (reg == "LR" || reg == "CTR")
				{
					const ullong reg_value = std::stoull(value.substr(16, 31), 0, 16);
					if (reg == "LR") ppu.lr = (u64)reg_value;
					if (reg == "CTR") ppu.ctr = (u64)reg_value;
					return;
				}
				if (reg == "CR")
				{
					const ullong reg_value = std::stoull(value.substr(24, 31), 0, 16);
					if (reg == "CR") ppu.cr_unpack((u32)reg_value);
					return;
				}
			}
			catch (std::invalid_argument&) //if any of the stoull conversion fail
			{
			}
		}
		else if (g_system == system_type::ps3 && cpu->id_type() != 1)
		{
			auto& spu = *static_cast<SPUThread*>(cpu);

			while (value.length() < 32) value = "0" + value;
			const auto first_brk = reg.find('[');
			try
			{
				if (first_brk != -1)
				{
					const long reg_index = std::atol(reg.substr(first_brk + 1, reg.length() - 2).c_str());
					if (reg.find("GPR") == 0)
					{
						const ullong reg_value0 = std::stoull(value.substr(16, 31), 0, 16);
						const ullong reg_value1 = std::stoull(value.substr(0, 15), 0, 16);
						spu.gpr[reg_index]._u64[0] = (u64)reg_value0;
						spu.gpr[reg_index]._u64[1] = (u64)reg_value1;
						return;
					}
				}
			}
			catch (std::invalid_argument&)
			{
			}
		}
		QMessageBox::critical(this, tr("Error"), tr("This value could not be converted.\nNo changes were made."));
	}
}

void RegisterEditorDialog::updateRegister()
{
	const auto cpu = this->cpu.lock();

	std::string reg = t1_register->itemData(t1_register->currentIndex()).toString().toUtf8().toStdString();
	std::string str;

	if (g_system == system_type::ps3 && cpu->id_type() == 1)
	{
		auto& ppu = *static_cast<ppu_thread*>(cpu.get());

		std::size_t first_brk = reg.find('[');
		if (first_brk != -1)
		{
			long reg_index = std::atol(reg.substr(first_brk + 1, reg.length() - first_brk - 2).c_str());
			if (reg.find("GPR") == 0) str = fmt::format("%016llx", ppu.gpr[reg_index]);
			if (reg.find("FPR") == 0) str = fmt::format("%016llx", ppu.fpr[reg_index]);
			if (reg.find("VR") == 0)  str = fmt::format("%016llx%016llx", ppu.vr[reg_index]._u64[1], ppu.vr[reg_index]._u64[0]);
		}
		if (reg == "CR")  str = fmt::format("%08x", ppu.cr_pack());
		if (reg == "LR")  str = fmt::format("%016llx", ppu.lr);
		if (reg == "CTR") str = fmt::format("%016llx", ppu.ctr);
	}
	else if (g_system == system_type::ps3 && cpu->id_type() != 1)
	{
		auto& spu = *static_cast<SPUThread*>(cpu.get());

		std::string::size_type first_brk = reg.find('[');
		if (first_brk != std::string::npos)
		{
			long reg_index;
			reg_index = atol(reg.substr(first_brk + 1, reg.length() - 2).c_str());
			if (reg.find("GPR") == 0) str = fmt::format("%016llx%016llx", spu.gpr[reg_index]._u64[1], spu.gpr[reg_index]._u64[0]);
		}
	}

	t2_value->setText(qstr(str));
}
