#include "register_editor_dialog.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>

constexpr auto qstr = QString::fromStdString;
inline std::string sstr(const QString& _in) { return _in.toStdString(); }
inline std::string sstr(const QVariant& _in) { return sstr(_in.toString()); }

register_editor_dialog::register_editor_dialog(QWidget *parent, u32 _pc, const std::shared_ptr<cpu_thread>& _cpu, CPUDisAsm* _disasm)
	: QDialog(parent)
	, m_pc(_pc)
	, m_disasm(_disasm)
	, cpu(_cpu)
{
	setWindowTitle(tr("Edit registers"));
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout* vbox_panel = new QVBoxLayout();
	QHBoxLayout* hbox_panel = new QHBoxLayout();
	QVBoxLayout* vbox_left_panel = new QVBoxLayout();
	QVBoxLayout* vbox_right_panel = new QVBoxLayout();
	QHBoxLayout* hbox_button_panel = new QHBoxLayout();

	QLabel* t1_text = new QLabel(tr("Register:     "), this);
	QLabel* t2_text = new QLabel(tr("Value (Hex):"), this);

	QPushButton* button_ok = new QPushButton(tr("&Ok"));
	QPushButton* button_cancel = new QPushButton(tr("&Cancel"));
	button_ok->setFixedWidth(80);
	button_cancel->setFixedWidth(80);

	m_register_combo = new QComboBox(this);
	m_value_line = new QLineEdit(this);
	m_value_line->setFixedWidth(200);

	// Layouts
	vbox_left_panel->addWidget(t1_text);
	vbox_left_panel->addWidget(t2_text);

	vbox_right_panel->addWidget(m_register_combo);
	vbox_right_panel->addWidget(m_value_line);

	hbox_button_panel->addWidget(button_ok);
	hbox_button_panel->addWidget(button_cancel);
	hbox_button_panel->setAlignment(Qt::AlignCenter);

	if (1)
	{
		if (_cpu->id_type() == 1)
		{
			for (int i = 0; i < 32; i++) m_register_combo->addItem(qstr(fmt::format("GPR[%d]", i)));
			for (int i = 0; i < 32; i++) m_register_combo->addItem(qstr(fmt::format("FPR[%d]", i)));
			for (int i = 0; i < 32; i++) m_register_combo->addItem(qstr(fmt::format("VR[%d]", i)));
			m_register_combo->addItem("CR");
			m_register_combo->addItem("LR");
			m_register_combo->addItem("CTR");
			//m_register_combo->addItem("XER");
			//m_register_combo->addItem("FPSCR");
		}
		else
		{
			for (int i = 0; i < 128; i++) m_register_combo->addItem(qstr(fmt::format("GPR[%d]", i)));
		}
	}

	// Main Layout
	hbox_panel->addLayout(vbox_left_panel);
	hbox_panel->addSpacing(10);
	hbox_panel->addLayout(vbox_right_panel);
	vbox_panel->addLayout(hbox_panel);
	vbox_panel->addSpacing(10);
	vbox_panel->addLayout(hbox_button_panel);
	setLayout(vbox_panel);
	setModal(true);

	// Events
	connect(button_ok, &QAbstractButton::clicked, this, [=, this](){OnOkay(_cpu); accept();});
	connect(button_cancel, &QAbstractButton::clicked, this, &register_editor_dialog::reject);
	connect(m_register_combo, &QComboBox::currentTextChanged, this, &register_editor_dialog::updateRegister);

	updateRegister(m_register_combo->currentText());
}

void register_editor_dialog::updateRegister(const QString& text)
{
	if (text.isEmpty())
	{
		m_value_line->setText("");
		return;
	}

	const auto cpu = this->cpu.lock();

	std::string reg = sstr(text);
	std::string str;

	if (cpu->id_type() == 1)
	{
		auto& ppu = *static_cast<ppu_thread*>(cpu.get());

		std::size_t first_brk = reg.find('[');
		if (first_brk != umax)
		{
			long reg_index = std::atol(reg.substr(first_brk + 1, reg.length() - first_brk - 2).c_str());
			if (reg.starts_with("GPR")) str = fmt::format("%016llx", ppu.gpr[reg_index]);
			if (reg.starts_with("FPR")) str = fmt::format("%016llx", ppu.fpr[reg_index]);
			if (reg.starts_with("VR"))  str = fmt::format("%016llx%016llx", ppu.vr[reg_index]._u64[1], ppu.vr[reg_index]._u64[0]);
		}
		if (reg == "CR")  str = fmt::format("%08x", ppu.cr.pack());
		if (reg == "LR")  str = fmt::format("%016llx", ppu.lr);
		if (reg == "CTR") str = fmt::format("%016llx", ppu.ctr);
	}
	else
	{
		auto& spu = *static_cast<spu_thread*>(cpu.get());

		std::string::size_type first_brk = reg.find('[');
		if (first_brk != umax)
		{
			long reg_index;
			reg_index = atol(reg.substr(first_brk + 1, reg.length() - 2).c_str());
			if (reg.starts_with("GPR")) str = fmt::format("%016llx%016llx", spu.gpr[reg_index]._u64[1], spu.gpr[reg_index]._u64[0]);
		}
	}

	m_value_line->setText(qstr(str));
}

void register_editor_dialog::OnOkay(const std::shared_ptr<cpu_thread>& _cpu)
{
	const auto cpu = _cpu.get();

	std::string reg = sstr(m_register_combo->currentText());
	std::string value = sstr(m_value_line->text());

	if (cpu->id_type() == 1)
	{
		auto& ppu = *static_cast<ppu_thread*>(cpu);

		while (value.length() < 32) value = "0" + value;
		const auto first_brk = reg.find('[');
		// TODO: handle invalid conversions
		{
			if (first_brk != umax)
			{
				const long reg_index = std::atol(reg.substr(first_brk + 1, reg.length() - first_brk - 2).c_str());
				if (reg.starts_with("GPR") || reg.starts_with("FPR"))
				{
					const ullong reg_value = std::stoull(value.substr(16, 31), 0, 16);
					if (reg.starts_with("GPR")) ppu.gpr[reg_index] = static_cast<u64>(reg_value);
					if (reg.starts_with("FPR")) ppu.fpr[reg_index] = std::bit_cast<f64>(reg_value);
					return;
				}
				if (reg.starts_with("VR"))
				{
					const ullong reg_value0 = std::stoull(value.substr(16, 31), 0, 16);
					const ullong reg_value1 = std::stoull(value.substr(0, 15), 0, 16);
					ppu.vr[reg_index]._u64[0] = static_cast<u64>(reg_value0);
					ppu.vr[reg_index]._u64[1] = static_cast<u64>(reg_value1);
					return;
				}
			}
			if (reg == "LR" || reg == "CTR")
			{
				const ullong reg_value = std::stoull(value.substr(16, 31), 0, 16);
				if (reg == "LR") ppu.lr = static_cast<u64>(reg_value);
				if (reg == "CTR") ppu.ctr = static_cast<u64>(reg_value);
				return;
			}
			if (reg == "CR")
			{
				const ullong reg_value = std::stoull(value.substr(24, 31), 0, 16);
				if (reg == "CR") ppu.cr.unpack(static_cast<u32>(reg_value));
				return;
			}
		}
	}
	else
	{
		auto& spu = *static_cast<spu_thread*>(cpu);

		while (value.length() < 32) value = "0" + value;
		const auto first_brk = reg.find('[');
		// TODO: handle invalid conversions
		{
			if (first_brk != umax)
			{
				const long reg_index = std::atol(reg.substr(first_brk + 1, reg.length() - 2).c_str());
				if (reg.starts_with("GPR"))
				{
					const ullong reg_value0 = std::stoull(value.substr(16, 31), 0, 16);
					const ullong reg_value1 = std::stoull(value.substr(0, 15), 0, 16);
					spu.gpr[reg_index]._u64[0] = static_cast<u64>(reg_value0);
					spu.gpr[reg_index]._u64[1] = static_cast<u64>(reg_value1);
					return;
				}
			}
		}
	}
	QMessageBox::critical(this, tr("Error"), tr("This value could not be converted.\nNo changes were made."));
}
