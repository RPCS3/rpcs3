#include "stdafx.h"
#include "save_data_utility.h"

//Cause i can not decide what struct to be used to fill those. Just use no real data now.
//Currently variable info isn't used. it supposed to be a container for the information passed by other.
save_data_info_dialog::save_data_info_dialog(QWidget* parent, const save_data_information& info)
	: QDialog(parent)
{
	setWindowTitle(tr("Save Data Information"));
	setMinimumSize(QSize(400, 300));

	// Table
	m_list = new QTableWidget(this);
	//m_list->setItemDelegate(new table_item_delegate(this)); // to get rid of item selection rectangles include "table_item_delegate.h"
	//m_list->setSelectionBehavior(QAbstractItemView::SelectRows); // enable to only select whole rows instead of items
	m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_list->setColumnCount(2);
	m_list->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Detail"));

	// Buttons
	QPushButton* close_button = new QPushButton(tr("&Close"), this);
	connect(close_button, &QAbstractButton::clicked, this, &save_data_info_dialog::close);

	// Button Layout
	QHBoxLayout* hbox_actions = new QHBoxLayout();
	hbox_actions->addStretch();	//Add a stretch to make Close on the Right-Down corner of this dialog.
	hbox_actions->addWidget(close_button);

	// Main Layout
	QVBoxLayout* vbox_main = new QVBoxLayout();
	vbox_main->addWidget(m_list, 1);
	vbox_main->addLayout(hbox_actions, 0);
	vbox_main->setAlignment(Qt::AlignCenter);
	setLayout(vbox_main);

	// resize to minimum view size
	resize(minimumSize().expandedTo(sizeHint()));

	UpdateData();
}
//This is intended to write the information of save data to QTableView.
//However been not able to decide which data struct i should use, i use static content for this to make it stub.
void save_data_info_dialog::UpdateData()
{
	m_list->clearContents();
	m_list->setRowCount(6); // set this to nr of members in struct

	m_list->setItem(0, 0, new QTableWidgetItem(tr("User ID")));
	m_list->setItem(0, 1, new QTableWidgetItem("00000000 (None)"));

	m_list->setItem(1, 0, new QTableWidgetItem(tr("Game Title")));
	m_list->setItem(1, 1, new QTableWidgetItem("Happy with rpcs3 (free)"));

	m_list->setItem(2, 0, new QTableWidgetItem(tr("Subtitle")));
	m_list->setItem(2, 1, new QTableWidgetItem("You devs are great"));

	m_list->setItem(3, 0, new QTableWidgetItem(tr("Detail")));
	m_list->setItem(3, 1, new QTableWidgetItem("Stub it first"));

	m_list->setItem(4, 0, new QTableWidgetItem(tr("Copyable")));
	m_list->setItem(4, 1, new QTableWidgetItem("1 (Not allowed)"));

	m_list->setItem(5, 0, new QTableWidgetItem(tr("Play Time")));
	m_list->setItem(5, 1, new QTableWidgetItem("00:00:00"));
	//Maybe there should be more details of save data.
	//But i'm getting bored for assign it one by one.

	m_list->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

//This dialog represents the Menu of Save Data Utility - which pop up after when you roll to a save and press triangle.
//I've ever thought of make it a right-click menu or a show-hide panel of the main dialog.
//Well only when those function calls related get implemented we can tell what this GUI should be, seriously.
save_data_manage_dialog::save_data_manage_dialog(QWidget* parent, unsigned int* sort_type, save_data_entry& save)
	: QDialog(parent)
{
	setWindowTitle(tr("Save Data Pop-up Menu"));
	setMinimumSize(QSize(400, 110));

	// "Sort By" ComboBox
	m_sort_options = new QComboBox(this);
	m_sort_options->setEditable(false);
	//You might change this - of corse we should know what to been set - maybe after functions related been implemented.
	m_sort_options->addItem(tr("User Id"));
	m_sort_options->addItem(tr("Game Title"));
	m_sort_options->addItem(tr("Game Subtitle"));
	m_sort_options->addItem(tr("Play Time"));
	m_sort_options->addItem(tr("Data Size"));
	m_sort_options->addItem(tr("Last Modified"));
	m_sort_options->addItem(tr("Created Time"));
	m_sort_options->addItem(tr("Accessed Time"));
	m_sort_options->addItem(tr("Modified Time"));
	m_sort_options->addItem(tr("Modify Time"));

	m_sort_type = sort_type;
	if (m_sort_type != nullptr)
	{
		//Check sort type and set it to combo box
		if (*m_sort_type >= m_sort_options->count())
		{
			*m_sort_type = 0;
		}
	}
	m_sort_options->setCurrentIndex(*m_sort_type);
	
	// Buttons
	QPushButton* pb_sort_action = new QPushButton(tr("&Apply"), this);
	QPushButton* pb_copy = new QPushButton(tr("&Copy"), this);
	QPushButton* pb_delete = new QPushButton(tr("&Delete"), this);
	QPushButton* pb_info = new QPushButton(tr("&Info"), this);
	QPushButton* pb_close = new QPushButton(tr("&Close"), this);

	// Sort Layout
	QHBoxLayout* hbox_sort = new QHBoxLayout();
	hbox_sort->setAlignment(Qt::AlignCenter);
	hbox_sort->addWidget(new QLabel(tr("Sort By"), this));
	hbox_sort->addWidget(m_sort_options);
	hbox_sort->addWidget(pb_sort_action);
	
	// Button Layout
	QHBoxLayout* hbox_actions = new QHBoxLayout();
	hbox_actions->setAlignment(Qt::AlignCenter);
	hbox_actions->addWidget(pb_copy);
	hbox_actions->addWidget(pb_delete);
	hbox_actions->addWidget(pb_info);
	hbox_actions->addWidget(pb_close);

	// Main Layout
	QVBoxLayout* vbox_manage = new QVBoxLayout();
	vbox_manage->setAlignment(Qt::AlignCenter);
	vbox_manage->addLayout(hbox_sort);
	vbox_manage->addLayout(hbox_actions);
	setLayout(vbox_manage);

	// Events
	connect(pb_sort_action, &QAbstractButton::clicked, this, &save_data_manage_dialog::OnApplySort);
	connect(pb_copy, &QAbstractButton::clicked, this, &save_data_manage_dialog::OnCopy);
	connect(pb_delete, &QAbstractButton::clicked, this, &save_data_manage_dialog::OnDelete);
	connect(pb_info, &QAbstractButton::clicked, this, &save_data_manage_dialog::OnInfo);
	connect(pb_close, &QAbstractButton::clicked, this, &save_data_manage_dialog::close);
}
//Display information about the current selected save data.
//If selected is "New Save Data" or other invalid, this dialog would be initialized with "Info" disabled or not visible.
void save_data_manage_dialog::OnInfo()
{
	LOG_WARNING(HLE, "Stub - save_data_utility: save_data_manage_dialog: OnInfo called.");
	save_data_information info;	//It should get a real one for information.. finally
	save_data_info_dialog* infoDialog = new save_data_info_dialog(this, info);
	infoDialog->setModal(true);
	infoDialog->show();
}
//Copy selected save data to another. Might need a dialog but i just leave it as this. Or Modal Dialog.
void save_data_manage_dialog::OnCopy()
{
	LOG_WARNING(HLE, "Stub - save_data_utility: save_data_manage_dialog: OnCopy called.");
}
//Delete selected save data, need confirm. just a stub now.
void save_data_manage_dialog::OnDelete()
{
	LOG_WARNING(HLE, "Stub - save_data_utility: save_data_manage_dialog: OnDelete called.");
}
//This should return the sort setting of the save data list. Also not implemented really.
void save_data_manage_dialog::OnApplySort()
{	
	*m_sort_type = m_sort_options->currentIndex();
	LOG_WARNING(HLE, "Stub - save_data_utility: save_data_manage_dialog: OnApplySort called. NAME=%s",
		m_sort_options->itemText(m_sort_options->currentIndex()).toStdString().c_str());
}

//Show up the savedata list, either to choose one to save/load or to manage saves.
//I suggest to use function callbacks to give save data list or get save data entry. (Not implemented or stubbed)
save_data_list_dialog::save_data_list_dialog(QWidget* parent, bool enable_manage)
	: QDialog(parent)
{
	setWindowTitle(tr("Save Data Utility"));
	setMinimumSize(QSize(400, 400));

	QLabel* l_description = new QLabel(tr("This is only a stub for now. This doesn't work yet due to related functions not being implemented."), this);
	l_description->setWordWrap(400);

	// Table
	m_list = new QTableWidget(this);
	//m_list->setItemDelegate(new table_item_delegate(this)); // to get rid of cell selection rectangles include "table_item_delegate.h"
	//m_list->setSelectionBehavior(QAbstractItemView::SelectRows); // enable to only select whole rows instead of items
	m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_list->setContextMenuPolicy(Qt::CustomContextMenu);
	m_list->setColumnCount(3);
	m_list->setHorizontalHeaderLabels(QStringList() << tr("Game ID") << tr("Save ID") << tr("Detail"));

	// Button Layout
	QHBoxLayout* hbox_action = new QHBoxLayout();
	QPushButton *m_close = new QPushButton(tr("&Close"), this);

	//If do not need manage, hide it, like just a save data picker.
	if (!enable_manage)
	{
		QPushButton *m_select = new QPushButton(tr("&Select"), this);
		connect(m_select, &QAbstractButton::clicked, this, &save_data_list_dialog::OnSelect);
		hbox_action->addWidget(m_select);
		setWindowTitle(tr("Save Data Chooser"));
	}
	else {
		QPushButton *m_manage = new QPushButton(tr("&Manage"), this);
		connect(m_manage, &QAbstractButton::clicked, this, &save_data_list_dialog::OnManage);
		hbox_action->addWidget(m_manage);
	}

	hbox_action->addStretch();
	hbox_action->addWidget(m_close);

	// events
	connect(m_close, &QAbstractButton::clicked, this, &save_data_list_dialog::close);
	connect(m_list, &QTableWidget::itemClicked, this, &save_data_list_dialog::OnEntryInfo);
	connect(m_list, &QTableWidget::customContextMenuRequested, this, &save_data_list_dialog::ShowContextMenu);
	connect(m_list->horizontalHeader(), &QHeaderView::sectionClicked, [=](int col){
		// Sort entries, update columns and refresh the panel. Taken from game_list_frame
		m_sortColumn = col;
		OnSort(m_sortColumn);
		UpdateList();
	});

	// main layout
	QVBoxLayout* vbox_main = new QVBoxLayout();
	vbox_main->setAlignment(Qt::AlignCenter);
	vbox_main->addWidget(l_description);
	vbox_main->addWidget(m_list);
	vbox_main->addLayout(hbox_action);
	setLayout(vbox_main);

	LoadEntries();
	UpdateList();
}
//After you pick a menu item from the sort sub-menu
void save_data_list_dialog::OnSort(int id)
{
	int idx = id;
	LOG_WARNING(HLE, "Stub - save_data_utility: save_data_list_dialog: OnSort called. Type Value:%d", idx);
	if ((idx < m_sort_type_count) && (idx >= 0))
	{
		m_sort_type = idx;

		if (m_sort_type == m_sortColumn)
		{
			m_sortAscending ^= true;
		}
		else
		{
			m_sortAscending = true;
		}
		// someSort(m_sort_type, m_sortAscending)
		// look at game_list_frame sortGameData for reference
	}
}
//Copy a existing save, need to get more arguments. maybe a new dialog.
void save_data_list_dialog::OnEntryCopy()
{
	int idx = m_list->currentRow();
	if (idx != -1)
	{
		LOG_WARNING(HLE, "Stub - save_data_utility: save_data_list_dialog: OnEntryCopy called.");
		//Some Operations?
		UpdateList();
	}
}
//Remove a save file, need to be confirmed.
void save_data_list_dialog::OnEntryRemove()
{
	int idx = m_list->currentRow();
	if (idx != -1)
	{
		LOG_WARNING(HLE, "Stub - save_data_utility: save_data_list_dialog: OnEntryRemove called.");
		//Some Operations?
		UpdateList();
	}
}
//Display info dialog directly.
void save_data_list_dialog::OnEntryInfo()
{
	int idx = m_list->currentRow();
	if (idx != -1)
	{
		LOG_WARNING(HLE, "Stub - save_data_utility: save_data_list_dialog: OnEntryInfo called.");
		save_data_information info;	//Only a stub now.
		save_data_info_dialog* infoDialog = new save_data_info_dialog(this, info);
		infoDialog->setModal(true);
		infoDialog->show();
	}
}
//Display info dialog directly.
void save_data_list_dialog::OnManage()
{
	int idx = m_list->currentRow();
	if (idx != -1)
	{
		LOG_WARNING(HLE, "Stub - save_data_utility: save_data_list_dialog: OnManage called.");
		save_data_entry save;	//Only a stub now.
		save_data_manage_dialog* manageDialog = new save_data_manage_dialog(this, &m_sort_type, save);
		manageDialog->setModal(true);
		manageDialog->show();
	}
}
//When you press that select button in the Chooser mode.
void save_data_list_dialog::OnSelect()
{
	int idx = m_list->currentRow();
	if (idx != -1)
	{
		LOG_WARNING(HLE, "Stub - save_data_utility: save_data_list_dialog: OnSelect called.");
		setModal(false);
	}
}
//Pop-up a small context-menu, being a replacement for save_data_manage_dialog
void save_data_list_dialog::ShowContextMenu(const QPoint &pos)
{
	QPoint globalPos = m_list->mapToGlobal(pos);
	QMenu* menu = new QMenu();
	int idx = m_list->currentRow();

	userIDAct = new QAction(tr("UserID"), this);
	titleAct = new QAction(tr("Title"), this);
	subtitleAct = new QAction(tr("Subtitle"), this);
	copyAct = new QAction(tr("&Copy"), this);
	removeAct = new QAction(tr("&Remove"), this);
	infoAct = new QAction(tr("&Info"), this);

	//This is also a stub for the sort setting. Ids is set according to their sort-type integer.
	m_sort_options = new QMenu(tr("&Sort"));
	m_sort_options->addAction(userIDAct);
	m_sort_options->addAction(titleAct);
	m_sort_options->addAction(subtitleAct);
	m_sort_type_count = 3; // set this !!!

	menu->addMenu(m_sort_options);
	menu->addSeparator();
	menu->addAction(copyAct);
	menu->addAction(removeAct);
	menu->addSeparator();
	menu->addAction(infoAct);

	copyAct->setEnabled(idx != -1);
	removeAct->setEnabled(idx != -1);

	//Events
	connect(copyAct, &QAction::triggered, this, &save_data_list_dialog::OnEntryCopy);
	connect(removeAct, &QAction::triggered, this, &save_data_list_dialog::OnEntryRemove);
	connect(infoAct, &QAction::triggered, this, &save_data_list_dialog::OnEntryInfo);

	connect(userIDAct, &QAction::triggered, this, [=] {OnSort(0); });
	connect(titleAct, &QAction::triggered, this, [=] {OnSort(1); });
	connect(subtitleAct, &QAction::triggered, this, [=] {OnSort(2); });

	menu->exec(globalPos);
}
//This is intended to load the save data list from a way. However that is not certain for a stub. Does nothing now.
void save_data_list_dialog::LoadEntries(void)
{

}
//Setup some static items just for display.
void save_data_list_dialog::UpdateList(void)
{
	m_list->clearContents();
	m_list->setRowCount(2); // set this to number of entries

	m_list->setItem(0, 0, new QTableWidgetItem("TEST00000"));
	m_list->setItem(0, 1, new QTableWidgetItem("00"));
	m_list->setItem(0, 2, new QTableWidgetItem("Final battle"));

	m_list->setItem(1, 0, new QTableWidgetItem("XXXX99876"));
	m_list->setItem(1, 1, new QTableWidgetItem("30"));
	m_list->setItem(1, 2, new QTableWidgetItem("This is a fake game"));

	m_list->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}
