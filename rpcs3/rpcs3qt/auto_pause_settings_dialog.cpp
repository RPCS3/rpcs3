#include "auto_pause_settings_dialog.h"
#include "table_item_delegate.h"
#include "Emu/System.h"

#include <QFontDatabase>
#include <QMenu>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QPushButton>

#include "util/logs.hpp"
#include "Utilities/File.h"

LOG_CHANNEL(autopause_log, "AutoPause");

auto_pause_settings_dialog::auto_pause_settings_dialog(QWidget *parent) : QDialog(parent)
{
	QLabel *description = new QLabel(tr("To use auto pause: enter the ID(s) of a function or a system call.\nRestart of the game is required to apply. You can enable/disable this in the settings."), this);

	m_pause_list = new QTableWidget(this);
	m_pause_list->setColumnCount(2);
	m_pause_list->setHorizontalHeaderLabels(QStringList() << tr("Call ID") << tr("Type"));
	//m_pause_list->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_pause_list->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_pause_list->setContextMenuPolicy(Qt::CustomContextMenu);
	m_pause_list->setItemDelegate(new table_item_delegate(this));
	m_pause_list->setShowGrid(false);

	QPushButton *clearButton = new QPushButton(tr("Clear"), this);
	QPushButton *reloadButton = new QPushButton(tr("Reload"), this);
	QPushButton *saveButton = new QPushButton(tr("Save"), this);
	QPushButton *cancelButton = new QPushButton(tr("Cancel"), this);
	cancelButton->setDefault(true);

	QHBoxLayout *buttonsLayout = new QHBoxLayout();
	buttonsLayout->addWidget(clearButton);
	buttonsLayout->addWidget(reloadButton);
	buttonsLayout->addStretch();
	buttonsLayout->addWidget(saveButton);
	buttonsLayout->addWidget(cancelButton);

	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(description);
	mainLayout->addWidget(m_pause_list);
	mainLayout->addLayout(buttonsLayout);
	setLayout(mainLayout);

	setMinimumSize(QSize(400, 360));
	setWindowTitle(tr("Auto Pause Manager"));
	setObjectName("auto_pause_manager");

	// Events
	connect(m_pause_list, &QTableWidget::customContextMenuRequested, this, &auto_pause_settings_dialog::ShowContextMenu);
	connect(clearButton, &QAbstractButton::clicked, [this](){ m_entries.clear(); UpdateList(); });
	connect(reloadButton, &QAbstractButton::clicked, [this](){ LoadEntries(); UpdateList(); });
	connect(saveButton, &QAbstractButton::clicked, [this]()
	{
		SaveEntries();
		autopause_log.success("File pause.bin was updated.");
	});
	connect(cancelButton, &QAbstractButton::clicked, this, &QWidget::close);

	Emu.GracefulShutdown(false);

	LoadEntries();
	UpdateList();
	setFixedSize(sizeHint());
}

// Copied some from AutoPause.
void auto_pause_settings_dialog::LoadEntries()
{
	m_entries.clear();
	m_entries.reserve(16);

	const fs::file list(fs::get_config_dir() + "pause.bin");

	if (list)
	{
		// System calls ID and Function calls ID are all u32 iirc.
		u32 num;
		const usz fmax = list.size();
		usz fcur = 0;
		list.seek(0);
		while (fcur <= fmax - sizeof(u32))
		{
			list.read(&num, sizeof(u32));
			fcur += sizeof(u32);
			if (num == 0xFFFFFFFF) break;

			m_entries.emplace_back(num);
		}
	}
}

// Copied some from AutoPause.
// Tip: This one doesn't check for the file is being read or not.
// This would always use a 0xFFFFFFFF as end of the pause.bin
void auto_pause_settings_dialog::SaveEntries()
{
	fs::file list(fs::get_config_dir() + "pause.bin", fs::rewrite);
	//System calls ID and Function calls ID are all u32 iirc.
	u32 num = 0;
	list.seek(0);
	for (usz i = 0; i < m_entries.size(); ++i)
	{
		if (num == 0xFFFFFFFF) continue;
		num = m_entries[i];
		list.write(&num, sizeof(u32));
	}
	num = 0xFFFFFFFF;
	list.write(&num, sizeof(u32));
}

void auto_pause_settings_dialog::UpdateList()
{
	const int entries_size = static_cast<int>(m_entries.size());
	m_pause_list->clearContents();
	m_pause_list->setRowCount(entries_size);
	for (int i = 0; i < entries_size; ++i)
	{
		QTableWidgetItem* callItem = new QTableWidgetItem;
		QTableWidgetItem* typeItem = new QTableWidgetItem;
		callItem->setFlags(callItem->flags() & ~Qt::ItemIsEditable);
		typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
		if (m_entries[i] != 0xFFFFFFFF)
		{
			callItem->setData(Qt::DisplayRole, QString::fromStdString(fmt::format("%08x", m_entries[i])));
		}
		else
		{
			callItem->setData(Qt::DisplayRole, tr("Unset"));
		}

		if (m_entries[i] < 1024)
		{
			typeItem->setData(Qt::DisplayRole, tr("System Call"));
		}
		else
		{
			typeItem->setData(Qt::DisplayRole, tr("Function Call"));
		}

		m_pause_list->setItem(i, 0, callItem);
		m_pause_list->setItem(i, 1, typeItem);
	}
}

void auto_pause_settings_dialog::ShowContextMenu(const QPoint &pos)
{
	const int row = m_pause_list->indexAt(pos).row();

	QMenu myMenu;

	// Make Actions
	QAction* add = myMenu.addAction(tr("&Add"));
	QAction* remove = myMenu.addAction(tr("&Remove"));
	myMenu.addSeparator();
	QAction* config = myMenu.addAction(tr("&Config"));

	if (row == -1)
	{
		remove->setEnabled(false);
		config->setEnabled(false);
	}

	auto OnEntryConfig = [this](int row, bool newEntry)
	{
		AutoPauseConfigDialog *config = new AutoPauseConfigDialog(this, this, newEntry, &m_entries[row]);
		config->setModal(true);
		config->exec();
		UpdateList();
	};

	connect(add, &QAction::triggered, this, [=, this]()
	{
		m_entries.emplace_back(0xFFFFFFFF);
		UpdateList();
		const int idx = static_cast<int>(m_entries.size()) - 1;
		m_pause_list->selectRow(idx);
		OnEntryConfig(idx, true);
	});
	connect(remove, &QAction::triggered, this, &auto_pause_settings_dialog::OnRemove);
	connect(config, &QAction::triggered, this, [=, this]() {OnEntryConfig(row, false); });

	myMenu.exec(m_pause_list->viewport()->mapToGlobal(pos));
}

void auto_pause_settings_dialog::OnRemove()
{
	QModelIndexList selection = m_pause_list->selectionModel()->selectedRows();
	std::sort(selection.begin(), selection.end());
	for (int i = selection.count() - 1; i >= 0; i--)
	{
		m_entries.erase(m_entries.begin() + ::at32(selection, i).row());
	}
	UpdateList();
}

void auto_pause_settings_dialog::keyPressEvent(QKeyEvent *event)
{
	if (event->isAutoRepeat())
	{
		return;
	}

	if (event->key() == Qt::Key_Delete)
	{
		OnRemove();
	}
}

AutoPauseConfigDialog::AutoPauseConfigDialog(QWidget* parent, auto_pause_settings_dialog* apsd, bool newEntry, u32 *entry)
	: QDialog(parent), m_presult(entry), m_newEntry(newEntry), m_apsd(apsd)
{
	m_entry = *m_presult;
	setMinimumSize(QSize(300, -1));

	QPushButton* button_ok = new QPushButton(tr("&Ok"), this);
	QPushButton* button_cancel = new QPushButton(tr("&Cancel"), this);
	button_ok->setFixedWidth(50);
	button_cancel->setFixedWidth(50);

	QLabel* description = new QLabel(tr("Specify ID of System Call or Function Call below. You need to use a Hexadecimal ID."), this);
	description->setWordWrap(true);

	m_current_converted = new QLabel(tr("Currently it gets an id of \"Unset\"."), this);
	m_current_converted->setWordWrap(true);

	m_id = new QLineEdit(this);
	m_id->setText(QString::fromStdString(fmt::format("%08x", m_entry)));
	m_id->setPlaceholderText("ffffffff");
	m_id->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	m_id->setMaxLength(8);
	m_id->setFixedWidth(65);
	setWindowTitle("Auto Pause Setting: " + m_id->text());

	connect(button_cancel, &QAbstractButton::clicked, this, &AutoPauseConfigDialog::OnCancel);
	connect(button_ok, &QAbstractButton::clicked, this, &AutoPauseConfigDialog::OnOk);
	connect(m_id, &QLineEdit::textChanged, this, &AutoPauseConfigDialog::OnUpdateValue);

	QHBoxLayout* configHBox = new QHBoxLayout();
	configHBox->addWidget(m_id);
	configHBox->addWidget(button_ok);
	configHBox->addWidget(button_cancel);
	configHBox->setAlignment(Qt::AlignCenter);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(description);
	mainLayout->addLayout(configHBox);
	mainLayout->addWidget(m_current_converted);

	setLayout(mainLayout);
	setFixedSize(QSize(300, sizeHint().height()));

	OnUpdateValue();
}

void AutoPauseConfigDialog::OnOk()
{
	bool ok;
	const ullong value = m_id->text().toULongLong(&ok, 16);

	m_entry = value;
	*m_presult = m_entry;

	accept();
}

void AutoPauseConfigDialog::OnCancel()
{
	if (m_newEntry)
	{
		m_apsd->OnRemove();
	}
	close();
}

void AutoPauseConfigDialog::OnUpdateValue() const
{
	bool ok;
	const ullong value = m_id->text().toULongLong(&ok, 16);
	const bool is_ok = ok && value <= u32{umax};

	m_current_converted->setText(tr("Current value: %1 (%2)").arg(value, 8, 16).arg(is_ok ? tr("OK") : tr("Conversion failed")));
}
