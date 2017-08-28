
#include "auto_pause_settings_dialog.h"

constexpr auto qstr = QString::fromStdString;

auto_pause_settings_dialog::auto_pause_settings_dialog(QWidget *parent) : QDialog(parent)
{
	QLabel *description = new QLabel(tr("To use auto pause: enter the ID(s) of a function or a system call.\nRestart of the game is required to apply. You can enable/disable this in the settings."), this);

	pauseList = new QTableWidget(this);
	pauseList->setColumnCount(2);
	pauseList->setHorizontalHeaderLabels(QStringList() << tr("Call ID") << tr("Type"));
	//pauseList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	pauseList->setSelectionBehavior(QAbstractItemView::SelectRows);
	pauseList->setContextMenuPolicy(Qt::CustomContextMenu);
	pauseList->setItemDelegate(new table_item_delegate(this));
	pauseList->setShowGrid(false);

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
	mainLayout->addWidget(pauseList);
	mainLayout->addLayout(buttonsLayout);
	setLayout(mainLayout);

	setMinimumSize(QSize(400, 360));
	setWindowTitle(tr("Auto Pause Manager"));

	//Events
	connect(pauseList, &QTableWidget::customContextMenuRequested, this, &auto_pause_settings_dialog::ShowContextMenu);
	connect(clearButton, &QAbstractButton::clicked, [=](){ m_entries.clear(); UpdateList(); });
	connect(reloadButton, &QAbstractButton::clicked, [=](){ LoadEntries(); UpdateList(); });
	connect(saveButton, &QAbstractButton::clicked, [=](){
		SaveEntries();
		LOG_SUCCESS(HLE, "Auto Pause: File pause.bin was updated.");
	});
	connect(cancelButton, &QAbstractButton::clicked, this, &QWidget::close);

	Emu.Stop();

	LoadEntries();
	UpdateList();
	setFixedSize(sizeHint());
}

//Copied some from AutoPause.
void auto_pause_settings_dialog::LoadEntries(void)
{
	m_entries.clear();
	m_entries.reserve(16);

	fs::file list(fs::get_config_dir() + "pause.bin");

	if (list)
	{
		//System calls ID and Function calls ID are all u32 iirc.
		u32 num;
		size_t fmax = list.size();
		size_t fcur = 0;
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

//Copied some from AutoPause.
//Tip: This one doesn't check for the file is being read or not.
//This would always use a 0xFFFFFFFF as end of the pause.bin
void auto_pause_settings_dialog::SaveEntries(void)
{
	fs::file list(fs::get_config_dir() + "pause.bin", fs::rewrite);
	//System calls ID and Function calls ID are all u32 iirc.
	u32 num = 0;
	list.seek(0);
	for (size_t i = 0; i < m_entries.size(); ++i)
	{
		if (num == 0xFFFFFFFF) continue;
		num = m_entries[i];
		list.write(&num, sizeof(u32));
	}
	num = 0xFFFFFFFF;
	list.write(&num, sizeof(u32));
}

void auto_pause_settings_dialog::UpdateList(void)
{
	const int entries_size = static_cast<int>(m_entries.size());
	pauseList->clearContents();
	pauseList->setRowCount(entries_size);
	for (int i = 0; i < entries_size; ++i)
	{
		QTableWidgetItem* callItem = new QTableWidgetItem;
		QTableWidgetItem* typeItem = new QTableWidgetItem;
		callItem->setFlags(callItem->flags() & ~Qt::ItemIsEditable);
		typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
		if (m_entries[i] != 0xFFFFFFFF)
		{	
			callItem->setData(Qt::DisplayRole, qstr(fmt::format("%08x", m_entries[i])));
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

		pauseList->setItem(i, 0, callItem);
		pauseList->setItem(i, 1, typeItem);
	}
}

void auto_pause_settings_dialog::ShowContextMenu(const QPoint &pos)
{
	int row = pauseList->indexAt(pos).row();

	QPoint globalPos = pauseList->mapToGlobal(pos);
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

	auto OnEntryConfig = [=](int row, bool newEntry)
	{
		AutoPauseConfigDialog *config = new AutoPauseConfigDialog(this, this, newEntry, &m_entries[row]);
		config->setModal(true);
		config->exec();
		UpdateList();
	};

	connect(add, &QAction::triggered, [=]() {
		m_entries.emplace_back(0xFFFFFFFF);
		UpdateList();
		int idx = static_cast<int>(m_entries.size()) - 1;
		pauseList->selectRow(idx);
		OnEntryConfig(idx, true);
	});
	connect(remove, &QAction::triggered, this, &auto_pause_settings_dialog::OnRemove);
	connect(config, &QAction::triggered, [=]() {OnEntryConfig(row, false); });

	myMenu.exec(globalPos);
}

void auto_pause_settings_dialog::OnRemove()
{
	QModelIndexList selection = pauseList->selectionModel()->selectedRows();
	std::sort(selection.begin(), selection.end());
	for (int i = selection.count() - 1; i >= 0; i--)
	{
		m_entries.erase(m_entries.begin() + selection.at(i).row());
	}
	UpdateList();
}

void auto_pause_settings_dialog::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Delete)
	{
		OnRemove();
	}
}

AutoPauseConfigDialog::AutoPauseConfigDialog(QWidget* parent, auto_pause_settings_dialog* apsd, bool newEntry, u32 *entry) 
	: QDialog(parent), m_presult(entry), b_newEntry(newEntry), apsd_parent(apsd)
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
	m_id->setText(qstr(fmt::format("%08x", m_entry)));
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
	ullong value = m_id->text().toULongLong(&ok, 16);

	m_entry = value;
	*m_presult = m_entry;

	accept();
}

void AutoPauseConfigDialog::OnCancel()
{
	if (b_newEntry)
	{
		apsd_parent->OnRemove();
	}
	close();
}

void AutoPauseConfigDialog::OnUpdateValue()
{
	bool ok;
	ullong value = m_id->text().toULongLong(&ok, 16);
	const bool is_ok = ok && value <= UINT32_MAX;
	
	m_current_converted->setText(qstr(fmt::format("Current value: %08x (%s)", u32(value), is_ok ? "OK" : "conversion failed")));
}
