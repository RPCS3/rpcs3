#include "elf_memory_dumping_dialog.h"
#include "Emu/Cell/SPUThread.h"

#include "qt_utils.h"

#include <QFileDialog>
#include <QCoreApplication>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QLineEdit>
#include <QLabel>

LOG_CHANNEL(gui_log, "GUI");

Q_DECLARE_METATYPE(spu_memory_segment_dump_data);

elf_memory_dumping_dialog::elf_memory_dumping_dialog(u32 ppu_debugger_addr, std::shared_ptr<gui_settings> _gui_settings, QWidget* parent)
	: QDialog(parent), m_gui_settings(std::move(_gui_settings))
{
	setWindowTitle(tr("SPU ELF Dumper"));
	setAttribute(Qt::WA_DeleteOnClose);

	m_seg_list = new QListWidget();

	// Font
	const int pSize = 10;
	QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	mono.setPointSize(pSize);

	m_seg_list->setMinimumWidth(gui::utils::get_label_width(tr("PPU Address: 0x00000000, LS Address: 0x00000, Segment Size: 0x00000, Flags: 0x0")));

	// Address expression input
	auto make_hex_edit = [this, mono](u32 max_digits)
	{
		QLineEdit* le = new QLineEdit();
		le->setFont(mono);
		le->setMaxLength(max_digits + 2);
		le->setPlaceholderText("0x" + QStringLiteral("0").repeated(max_digits));
		le->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^(0[xX])?0*[a-fA-F0-9]{0,%1}$").arg(max_digits)), this));
		return le;
	};

	m_segment_size_input = make_hex_edit(5);
	m_ppu_address_input = make_hex_edit(8);
	m_ls_address_input = make_hex_edit(5);
	m_segment_flags_input = make_hex_edit(1);
	m_segment_flags_input->setText("0x7"); // READ WRITE EXEC
	m_ppu_address_input->setText(QStringLiteral("0x%1").arg(ppu_debugger_addr & -0x10000, 1, 16)); // SPU code segments are usually 128 bytes aligned, let's make it even 64k so the user would have to type himself the lower part to avoid human errors.

	QPushButton* add_segment_button = new QPushButton(QStringLiteral("+"));
	add_segment_button->setToolTip(tr("Add new segment"));
	add_segment_button->setFixedWidth(add_segment_button->sizeHint().height()); // Make button square
	connect(add_segment_button, &QAbstractButton::clicked, this, &elf_memory_dumping_dialog::add_new_segment);

	QPushButton* remove_segment_button = new QPushButton(QStringLiteral("-"));
	remove_segment_button->setToolTip(tr("Remove segment"));
	remove_segment_button->setFixedWidth(remove_segment_button->sizeHint().height()); // Make button square
	remove_segment_button->setEnabled(false);
	connect(remove_segment_button, &QAbstractButton::clicked, this, &elf_memory_dumping_dialog::remove_segment);

	QPushButton* save_to_file = new QPushButton(tr("Save To ELF"));
	save_to_file->setToolTip(tr("Save To An ELF file"));
	connect(save_to_file, &QAbstractButton::clicked, this, &elf_memory_dumping_dialog::save_to_file);

	QHBoxLayout* hbox_input = new QHBoxLayout;
	hbox_input->addWidget(new QLabel(tr("Segment Size:")));
	hbox_input->addWidget(m_segment_size_input);
	hbox_input->addSpacing(5);

	hbox_input->addWidget(new QLabel(tr("PPU Address:")));
	hbox_input->addWidget(m_ppu_address_input);
	hbox_input->addSpacing(5);
	hbox_input->addWidget(new QLabel(tr("LS Address:")));
	hbox_input->addWidget(m_ls_address_input);
	hbox_input->addSpacing(5);
	hbox_input->addWidget(new QLabel(tr("Flags:")));
	hbox_input->addWidget(m_segment_flags_input);

	QHBoxLayout* hbox_save_and_edit = new QHBoxLayout;
	hbox_save_and_edit->addStretch(2);
	hbox_save_and_edit->addWidget(add_segment_button);
	hbox_save_and_edit->addSpacing(4);
	hbox_save_and_edit->addWidget(remove_segment_button);
	hbox_save_and_edit->addSpacing(4);
	hbox_save_and_edit->addWidget(save_to_file);

	QVBoxLayout* vbox = new QVBoxLayout();
	vbox->addLayout(hbox_input);
	vbox->addSpacing(5);
	vbox->addWidget(m_seg_list);
	vbox->addSpacing(5);
	vbox->addLayout(hbox_save_and_edit);

	setLayout(vbox);

	connect(m_seg_list, &QListWidget::currentRowChanged, this, [this, remove_segment_button](int row)
	{
		remove_segment_button->setEnabled(row >= 0 && m_seg_list->item(row));
	});

	show();
}

void elf_memory_dumping_dialog::add_new_segment()
{
	QStringList errors;
	auto interpret = [&](QString text, QString error_field) -> u32
	{
		bool ok = false;

		// Parse expression (or at least used to, was nuked to remove the need for QtJsEngine)
		const QString fixed_expression = QRegularExpression(QRegularExpression::anchoredPattern("a .*|^[A-Fa-f0-9]+$")).match(text).hasMatch() ? "0x" + text : text;
		const u32 res = static_cast<u32>(fixed_expression.toULong(&ok, 16));

		if (!ok)
		{
			errors << error_field;
			return umax;
		}

		return res;
	};

	spu_memory_segment_dump_data data{};
	data.segment_size = interpret(m_segment_size_input->text(), tr("Segment Size"));
	data.src_addr = vm::get_super_ptr(interpret(m_ppu_address_input->text(), tr("PPU Address")));
	data.ls_addr = interpret(m_ls_address_input->text(), tr("LS Address"));
	data.flags = interpret(m_segment_flags_input->text(), tr("Segment Flags"));

	if (!errors.isEmpty())
	{
		QMessageBox::warning(this, tr("Failed To Add Segment"), tr("Segment parameters are incorrect:\n%1").arg(errors.join('\n')));
		return;
	}

	if (data.segment_size % 4)
	{
		QMessageBox::warning(this, tr("Failed To Add Segment"), tr("SPU segment size must be 4 bytes aligned."));
		return;
	}

	if (data.segment_size + data.ls_addr > SPU_LS_SIZE || data.segment_size == 0 || data.segment_size % 4)
	{
		QMessageBox::warning(this, tr("Failed To Add Segment"), tr("SPU segment range is invalid."));
		return;
	}

	if (!vm::check_addr(vm::try_get_addr(data.src_addr).first, vm::page_readable, data.segment_size))
	{
		QMessageBox::warning(this, tr("Failed To Add Segment"), tr("PPU address range is not accessible."));
		return;
	}

	for (int i = 0; i < m_seg_list->count(); ++i)
	{
		ensure(m_seg_list->item(i)->data(Qt::UserRole).canConvert<spu_memory_segment_dump_data>());
		const auto seg_stored = m_seg_list->item(i)->data(Qt::UserRole).value<spu_memory_segment_dump_data>();

		const auto stored_max = seg_stored.src_addr + seg_stored.segment_size;
		const auto data_max = data.src_addr + data.segment_size;

		if (seg_stored.src_addr < data_max && data.src_addr < stored_max)
		{
			QMessageBox::warning(this, tr("Failed To Add Segment"), tr("SPU segment overlaps with previous SPU segment(s)\n"));
			return;
		}
	}

	auto item = new QListWidgetItem(tr("PPU Address: 0x%0, LS Address: 0x%1, Segment Size: 0x%2, Flags: 0x%3").arg(+vm::try_get_addr(data.src_addr).first, 5, 16).arg(data.ls_addr, 2, 16).arg(data.segment_size, 2, 16).arg(data.flags, 2, 16), m_seg_list);
	item->setData(Qt::UserRole, QVariant::fromValue(data));
	m_seg_list->setCurrentItem(item);
}

void elf_memory_dumping_dialog::remove_segment()
{
	const int row = m_seg_list->currentRow();
	if (row >= 0)
	{
		QListWidgetItem* item = m_seg_list->takeItem(row);
		delete item;
	}
}

void elf_memory_dumping_dialog::save_to_file()
{
	std::vector<spu_memory_segment_dump_data> segs;
	segs.reserve(m_seg_list->count());

	for (int i = 0; i < m_seg_list->count(); ++i)
	{
		ensure(m_seg_list->item(i)->data(Qt::UserRole).canConvert<spu_memory_segment_dump_data>());
		const auto seg_stored = m_seg_list->item(i)->data(Qt::UserRole).value<spu_memory_segment_dump_data>();
		segs.emplace_back(seg_stored);
	}

	if (segs.empty())
	{
		return;
	}

	const QString path_last_elf = m_gui_settings->GetValue(gui::fd_save_elf).toString();

	const QString qpath = QFileDialog::getSaveFileName(this, tr("Capture"), path_last_elf, "SPU ELF (*.elf)" );
	const std::string path = qpath.toStdString();

	if (!path.empty())
	{
		const auto result = spu_thread::capture_memory_as_elf({segs.data(), segs.size()}).save();

		if (!result.empty() && fs::write_file(path, fs::rewrite, result))
		{
			gui_log.success("Saved ELF at %s", path);
			m_gui_settings->SetValue(gui::fd_save_elf, qpath);
		}
		else
		{
			QMessageBox::warning(this, tr("Save Failure"), tr("Failed to save SPU ELF."));
		}
	}
}
