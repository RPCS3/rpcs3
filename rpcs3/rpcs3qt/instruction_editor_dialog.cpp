
#include "instruction_editor_dialog.h"
#include <QFontDatabase>

constexpr auto qstr = QString::fromStdString;

instruction_editor_dialog::instruction_editor_dialog(QWidget *parent, u32 _pc, const std::shared_ptr<cpu_thread>& _cpu, CPUDisAsm* _disasm)
	: QDialog(parent)
	, pc(_pc)
	, cpu(_cpu)
	, disasm(_disasm)
{
	setWindowTitle(tr("Edit instruction"));
	setAttribute(Qt::WA_DeleteOnClose);
	setMinimumSize(300, sizeHint().height());

	const auto cpu = _cpu.get();
	cpu_offset = g_system == system_type::ps3 && cpu->id_type() != 1 ? static_cast<SPUThread&>(*cpu).offset : 0;
	QString instruction = qstr(fmt::format("%08x", vm::ps3::read32(cpu_offset + pc).value()));

	QVBoxLayout* vbox_panel(new QVBoxLayout());
	QHBoxLayout* hbox_panel(new QHBoxLayout());
	QVBoxLayout* vbox_left_panel(new QVBoxLayout());
	QVBoxLayout* vbox_right_panel(new QVBoxLayout());
	QHBoxLayout* hbox_b_panel(new QHBoxLayout());

	QPushButton* button_ok(new QPushButton(tr("Ok")));
	QPushButton* button_cancel(new QPushButton(tr("Cancel")));
	button_ok->setFixedWidth(80);
	button_cancel->setFixedWidth(80);

	QLabel* t1_text = new QLabel(tr("Address:     "), this);
	QLabel* t2_text = new QLabel(tr("Instruction: "), this);
	QLabel* t3_text = new QLabel(tr("Preview:     "), this);
	QLabel* t1_addr = new QLabel(qstr(fmt::format("%08x", pc)), this);
	t2_instr = new QLineEdit(this);
	t2_instr->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	t2_instr->setPlaceholderText(instruction);
	t2_instr->setText(instruction);
	t2_instr->setMaxLength(8);
	t2_instr->setMaximumWidth(65);
	t3_preview = new QLabel("", this);

	// Layouts
	vbox_left_panel->addWidget(t1_text);
	vbox_left_panel->addWidget(t2_text);
	vbox_left_panel->addWidget(t3_text);

	vbox_right_panel->addWidget(t1_addr);
	vbox_right_panel->addWidget(t2_instr);
	vbox_right_panel->addWidget(t3_preview);
	vbox_right_panel->setAlignment(Qt::AlignLeft);

	hbox_b_panel->addWidget(button_ok);
	hbox_b_panel->addWidget(button_cancel);
	hbox_b_panel->setAlignment(Qt::AlignCenter);

	// Main Layout
	hbox_panel->addLayout(vbox_left_panel);
	hbox_panel->addSpacing(10);
	hbox_panel->addLayout(vbox_right_panel);
	vbox_panel->addLayout(hbox_panel);
	vbox_panel->addSpacing(10);
	vbox_panel->addLayout(hbox_b_panel);
	setLayout(vbox_panel);
	setModal(true);

	// Events
	connect(button_ok, &QAbstractButton::pressed, [=]() {
		bool ok;
		ulong opcode = t2_instr->text().toULong(&ok, 16);
		if (!ok)
			QMessageBox::critical(this, tr("Error"), tr("This instruction could not be parsed.\nNo changes were made."));
		else
			vm::ps3::write32(cpu_offset + pc, (u32)opcode);
		accept();
	});
	connect(button_cancel, &QAbstractButton::pressed, this, &instruction_editor_dialog::reject);
	connect(t2_instr, &QLineEdit::textChanged, this, &instruction_editor_dialog::updatePreview);

	updatePreview();
}

void instruction_editor_dialog::updatePreview()
{
	bool ok;
	ulong opcode = t2_instr->text().toULong(&ok, 16);
	Q_UNUSED(opcode);

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
