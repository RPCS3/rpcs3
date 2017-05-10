
#include "InstructionEditor.h"

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }

InstructionEditorDialog::InstructionEditorDialog(QWidget *parent, u32 _pc, const std::shared_ptr<cpu_thread>& _cpu, CPUDisAsm* _disasm)
	: QDialog(parent)
	, pc(_pc)
	, cpu(_cpu)
	, disasm(_disasm)
{
	setWindowTitle(tr("Edit instruction"));

	QHBoxLayout* hbox_panel_margin_x(new QHBoxLayout());
	QVBoxLayout* vbox_panel_margin_y(new QVBoxLayout());

	QVBoxLayout* vbox_panel(new QVBoxLayout());
	QHBoxLayout* hbox_t1_panel(new QHBoxLayout());
	QHBoxLayout* hbox_t2_panel(new QHBoxLayout());
	QHBoxLayout* hbox_t3_panel(new QHBoxLayout());
	QHBoxLayout* hbox_b_panel(new QHBoxLayout());

	QPushButton* button_ok(new QPushButton(tr("Ok")));
	QPushButton* button_cancel(new QPushButton(tr("Cancel")));

	QLabel* t1_text = new QLabel(tr("Address:     "), this);
	QLabel* t1_addr = new QLabel(qstr(fmt::format("%08x", pc)), this);
	QLabel* t2_text = new QLabel(tr("Instruction:"), this);
	t2_instr = new QLineEdit(this);
	QLabel* t3_text = new QLabel(tr("Preview:     "), this);
	t3_preview = new QLabel("", this);

	hbox_t1_panel->addWidget(t1_text);
	hbox_t1_panel->addSpacing(8);
	hbox_t1_panel->addWidget(t1_addr);

	hbox_t2_panel->addWidget(t2_text);
	hbox_t2_panel->addSpacing(8);
	hbox_t2_panel->addWidget(t2_instr);

	hbox_t3_panel->addWidget(t3_text);
	hbox_t3_panel->addSpacing(8);
	hbox_t3_panel->addWidget(t3_preview);

	hbox_b_panel->addWidget(button_ok);
	hbox_b_panel->addSpacing(5);
	hbox_b_panel->addWidget(button_cancel);

	vbox_panel->addLayout(hbox_t1_panel);
	vbox_panel->addSpacing(8);
	vbox_panel->addLayout(hbox_t3_panel);
	vbox_panel->addSpacing(8);
	vbox_panel->addLayout(hbox_t2_panel);
	vbox_panel->addSpacing(16);
	vbox_panel->addLayout(hbox_b_panel);

	vbox_panel_margin_y->addSpacing(12);
	vbox_panel_margin_y->addLayout(vbox_panel);
	vbox_panel_margin_y->addSpacing(12);
	hbox_panel_margin_x->addSpacing(12);
	hbox_panel_margin_x->addLayout(vbox_panel_margin_y);
	hbox_panel_margin_x->addSpacing(12);

	const auto cpu = _cpu.get();

	const u32 cpu_offset = g_system == system_type::ps3 && cpu->id_type() != 1 ? static_cast<SPUThread&>(*cpu).offset : 0;

	connect(t2_instr, &QLineEdit::textChanged, this, &InstructionEditorDialog::updatePreview);
	t2_instr->setText(qstr(fmt::format("%08x", vm::ps3::read32(cpu_offset + pc).value())));

	setLayout(hbox_panel_margin_x);
	setModal(true);

	bool ok;
	ulong opcode = t2_instr->text().toULong(&ok, 16);
	if (!ok)
		QMessageBox::critical(this, tr("Error"), tr("This instruction could not be parsed.\nNo changes were made."));
	else
		vm::ps3::write32(cpu_offset + pc, (u32)opcode);
}

void InstructionEditorDialog::updatePreview()
{
	bool ok;
	ulong opcode = t2_instr->text().toULong(&ok, 16);
	if (ok)
	{
		if (g_system == system_type::psv)
		{
			t3_preview->setText(tr("Preview for ARMv7Thread not implemented yet."));
		}
		else
		{
			t3_preview->setText(tr("Preview disabled."));
		}
	}
	else
	{
		t3_preview->setText(tr("Could not parse instruction."));
	}
}
