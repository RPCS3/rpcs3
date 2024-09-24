#include "save_data_info_dialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QHeaderView>
#include "Emu/System.h"

save_data_info_dialog::save_data_info_dialog(SaveDataEntry save, QWidget* parent)
	: QDialog(parent)
	, m_entry(std::move(save))
{
	setWindowTitle(tr("Save Data Information"));

	// Table
	m_list = new QTableWidget(this);

	//m_list->setItemDelegate(new table_item_delegate(this)); // to get rid of item selection rectangles include "table_item_delegate.h"
	m_list->setSelectionBehavior(QAbstractItemView::SelectRows); // enable to only select whole rows instead of items
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

	UpdateData();

	m_list->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	m_list->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);

	const QSize table_size
	(
		m_list->verticalHeader()->width() + m_list->horizontalHeader()->length() + m_list->frameWidth() * 2,
		m_list->horizontalHeader()->height() + m_list->verticalHeader()->length() + m_list->frameWidth() * 2
	);

	// no minimum size needed because we always have same table size and row count
	resize(sizeHint() - m_list->sizeHint() + table_size);
}

//This is intended to write the information of save data to QTableView.
void save_data_info_dialog::UpdateData()
{
	m_list->clearContents();
	const int num_entries = 4; // set this to number of members in struct
	m_list->setRowCount(num_entries);

	//Maybe there should be more details of save data.
	m_list->setItem(0, 0, new QTableWidgetItem(tr("User ID")));
	m_list->setItem(0, 1, new QTableWidgetItem(QString::fromStdString(Emu.GetUsr())));

	m_list->setItem(1, 0, new QTableWidgetItem(tr("Title")));
	m_list->setItem(1, 1, new QTableWidgetItem(QString::fromStdString(m_entry.title)));

	m_list->setItem(2, 0, new QTableWidgetItem(tr("Subtitle")));
	m_list->setItem(2, 1, new QTableWidgetItem(QString::fromStdString(m_entry.subtitle)));

	m_list->setItem(3, 0, new QTableWidgetItem(tr("Detail")));
	m_list->setItem(3, 1, new QTableWidgetItem(QString::fromStdString(m_entry.details)));

	QImage img;
	if (!m_entry.iconBuf.empty() && img.loadFromData(m_entry.iconBuf.data(), static_cast<int>(m_entry.iconBuf.size()), "PNG"))
	{
		m_list->insertRow(0);
		QTableWidgetItem* img_item = new QTableWidgetItem();
		img_item->setData(Qt::DecorationRole, QPixmap::fromImage(img));
		m_list->setItem(0, 0, new QTableWidgetItem(tr("Icon")));
		m_list->setItem(0, 1, img_item);
	}
}
