#include "instruction_editor_dialog.h"
#include "hex_validator.h"

#include "Emu/Cell/SPUThread.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Cell/lv2/sys_spu.h"

#include <QFontDatabase>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QCheckBox>

extern bool ppu_patch(u32 addr, u32 value);

extern std::string format_spu_func_info(u32 addr, cpu_thread* spu);

instruction_editor_dialog::instruction_editor_dialog(QWidget *parent, u32 _pc, CPUDisAsm* _disasm, std::function<cpu_thread*()> func)
	: QDialog(parent)
	, m_pc(_pc)
	, m_disasm(_disasm->copy_type_erased())
	, m_get_cpu(func ? std::move(func) : std::function<cpu_thread*()>(FN(nullptr)))
{
	setWindowTitle(tr("Edit instruction"));
	setAttribute(Qt::WA_DeleteOnClose);
	setMinimumSize(300, sizeHint().height());

	const auto cpu = m_get_cpu();

	m_cpu_offset = cpu && cpu->get_class() == thread_class::spu ? static_cast<spu_thread&>(*cpu).ls : vm::g_sudo_addr;

	QVBoxLayout* vbox_panel(new QVBoxLayout());
	QHBoxLayout* hbox_panel(new QHBoxLayout());
	QVBoxLayout* vbox_left_panel(new QVBoxLayout());
	QVBoxLayout* vbox_right_panel(new QVBoxLayout());
	QHBoxLayout* hbox_b_panel(new QHBoxLayout());

	QPushButton* button_ok(new QPushButton(tr("OK")));
	QPushButton* button_cancel(new QPushButton(tr("Cancel")));
	button_ok->setFixedWidth(80);
	button_cancel->setFixedWidth(80);

	m_instr = new QLineEdit(this);
	m_instr->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	m_instr->setValidator(new HexValidator(m_instr));
	m_instr->setMaximumWidth(130);

	m_disasm->change_mode(cpu_disasm_mode::normal);
	m_disasm->disasm(m_pc);
	m_preview = new QLabel(QString::fromStdString(m_disasm->last_opcode), this);

	// Layouts
	vbox_left_panel->addWidget(new QLabel(tr("Address:     ")));
	vbox_left_panel->addWidget(new QLabel(tr("Instruction: ")));
	vbox_left_panel->addWidget(new QLabel(tr("Preview:     ")));

	vbox_right_panel->addWidget(new QLabel(QString::fromStdString(fmt::format("%08x", m_pc))));
	vbox_right_panel->addWidget(m_instr);
	vbox_right_panel->addWidget(m_preview);

	if (cpu && cpu->get_class() == thread_class::spu)
	{
		// Print block information as if this instruction is its beginning
		vbox_left_panel->addWidget(new QLabel(tr("Block Info:  ")));
		m_func_info = new QLabel("", this);
		vbox_right_panel->addWidget(m_func_info);

		m_func_info->setText(QString::fromStdString(format_spu_func_info(m_pc, cpu)));
	}

	if (cpu && cpu->get_class() == thread_class::spu)
	{
		const auto& spu = static_cast<spu_thread&>(*cpu);

		if (spu.group && spu.group->max_num > 1)
		{
			m_apply_for_spu_group = new QCheckBox(tr("For SPUs Group"));
			vbox_left_panel->addWidget(m_apply_for_spu_group);
			vbox_right_panel->addWidget(new QLabel("")); // For alignment
		}
	}

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

	// Events
	connect(button_ok, &QAbstractButton::clicked, [this]()
	{
		const auto cpu = m_get_cpu();

		if (!cpu)
		{
			close();
			return;
		}

		bool ok;
		const ulong opcode = normalize_hex_qstring(m_instr->text()).toULong(&ok, 16);
		if (!ok || opcode > u32{umax})
		{
			QMessageBox::critical(this, tr("Error"), tr("Failed to parse PPU instruction."));
			return;
		}

		const be_t<u32> swapped{static_cast<u32>(opcode)};

		if (cpu->get_class() == thread_class::ppu)
		{
			if (!ppu_patch(m_pc, static_cast<u32>(opcode)))
			{
				QMessageBox::critical(this, tr("Error"), tr("Failed to patch PPU instruction."));
				return;
			}
		}
		else if (m_apply_for_spu_group && m_apply_for_spu_group->isChecked())
		{
			for (auto& spu : static_cast<spu_thread&>(*cpu).group->threads)
			{
				if (spu)
				{
					std::memcpy(spu->ls + m_pc, &swapped, 4);
				}
			}
		}
		else
		{
			std::memcpy(m_cpu_offset + m_pc, &swapped, 4);
		}

		accept();
	});
	connect(button_cancel, &QAbstractButton::clicked, this, &instruction_editor_dialog::reject);
	connect(m_instr, &QLineEdit::textChanged, this, &instruction_editor_dialog::updatePreview);

	updatePreview();
}

void instruction_editor_dialog::updatePreview() const
{
	bool ok;
	const be_t<u32> opcode{static_cast<u32>(normalize_hex_qstring(m_instr->text()).toULong(&ok, 16))};
	m_disasm->change_ptr(reinterpret_cast<const u8*>(&opcode) - std::intptr_t{m_pc});

	if (ok && m_disasm->disasm(m_pc))
	{
		m_preview->setText(QString::fromStdString(m_disasm->last_opcode));
	}
	else
	{
		m_preview->setText(tr("Could not parse instruction."));
	}
}
