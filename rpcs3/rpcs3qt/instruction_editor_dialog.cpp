#include "instruction_editor_dialog.h"

#include "Emu/Cell/SPUThread.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"

#include <QFontDatabase>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>

constexpr auto qstr = QString::fromStdString;

extern bool ppu_patch(u32 addr, u32 value);

instruction_editor_dialog::instruction_editor_dialog(QWidget *parent, u32 _pc, const std::shared_ptr<cpu_thread>& _cpu, CPUDisAsm* _disasm)
	: QDialog(parent)
	, m_pc(_pc)
	, m_disasm(_disasm)
	, cpu(_cpu)
{
	setWindowTitle(tr("Edit instruction"));
	setAttribute(Qt::WA_DeleteOnClose);
	setMinimumSize(300, sizeHint().height());

	const auto cpu = _cpu.get();
	m_cpu_offset = cpu->id_type() != 1 ? static_cast<spu_thread&>(*cpu).offset : 0;
	QString instruction = qstr(fmt::format("%08x", vm::read32(m_cpu_offset + m_pc).value()));

	QVBoxLayout* vbox_panel(new QVBoxLayout());
	QHBoxLayout* hbox_panel(new QHBoxLayout());
	QVBoxLayout* vbox_left_panel(new QVBoxLayout());
	QVBoxLayout* vbox_right_panel(new QVBoxLayout());
	QHBoxLayout* hbox_b_panel(new QHBoxLayout());

	QPushButton* button_ok(new QPushButton(tr("Ok")));
	QPushButton* button_cancel(new QPushButton(tr("Cancel")));
	button_ok->setFixedWidth(80);
	button_cancel->setFixedWidth(80);

	m_instr = new QLineEdit(this);
	m_instr->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	m_instr->setPlaceholderText(instruction);
	m_instr->setText(instruction);
	m_instr->setMaxLength(8);
	m_instr->setMaximumWidth(65);

	m_preview = new QLabel("", this);

	// Layouts
	vbox_left_panel->addWidget(new QLabel(tr("Address:     ")));
	vbox_left_panel->addWidget(new QLabel(tr("Instruction: ")));
	vbox_left_panel->addWidget(new QLabel(tr("Preview:     ")));

	vbox_right_panel->addWidget(new QLabel(qstr(fmt::format("%08x", m_pc))));
	vbox_right_panel->addWidget(m_instr);
	vbox_right_panel->addWidget(m_preview);
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
	connect(button_ok, &QAbstractButton::clicked, [=, this]()
	{
		bool ok;
		ulong opcode = m_instr->text().toULong(&ok, 16);
		if (!ok || opcode > UINT32_MAX)
		{
			QMessageBox::critical(this, tr("Error"), tr("Failed to parse PPU instruction."));
			return;
		}
		else if (cpu->id_type() == 1)
		{
			if (!ppu_patch(m_cpu_offset + m_pc, static_cast<u32>(opcode)))
			{
				QMessageBox::critical(this, tr("Error"), tr("Failed to patch PPU instruction."));
				return;
			}
		}
		else
		{
			vm::write32(m_cpu_offset + m_pc, static_cast<u32>(opcode));
		}

		accept();
	});
	connect(button_cancel, &QAbstractButton::clicked, this, &instruction_editor_dialog::reject);
	connect(m_instr, &QLineEdit::textChanged, this, &instruction_editor_dialog::updatePreview);

	updatePreview();
}

void instruction_editor_dialog::updatePreview()
{
	bool ok;
	ulong opcode = m_instr->text().toULong(&ok, 16);
	Q_UNUSED(opcode);

	if (ok)
	{
		m_preview->setText(tr("Preview disabled."));
	}
	else
	{
		m_preview->setText(tr("Could not parse instruction."));
	}
}
