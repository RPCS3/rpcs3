#include "vfs_dialog_usb_tab.h"
#include "vfs_dialog_usb_input.h"
#include "table_item_delegate.h"
#include "gui_settings.h"
#include "Utilities/Config.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QMenu>
#include <QMouseEvent>

constexpr int max_usb_devices = 8;

const auto get_device_info = [](const QString& device_name, const cfg::map_of_type<cfg::device_info>& device_map) -> cfg::device_info
{
	if (auto it = device_map.find(device_name.toStdString()); it != device_map.cend())
	{
		return it->second;
	}

	return {};
};

const auto get_device_name = [](int i) -> QString
{
	return QString("/dev_usb00%0").arg(i);
};

enum usb_column : int
{
	usb_name   = 0,
	usb_path   = 1,
	usb_vid    = 2,
	usb_pid    = 3,
	usb_serial = 4
};

vfs_dialog_usb_tab::vfs_dialog_usb_tab(cfg::device_entry* cfg_node, std::shared_ptr<gui_settings> _gui_settings, QWidget* parent)
	: QWidget(parent), m_cfg_node(cfg_node), m_gui_settings(std::move(_gui_settings))
{
	m_usb_table = new QTableWidget(this);
	m_usb_table->setItemDelegate(new table_item_delegate(this, false));
	m_usb_table->setShowGrid(false);
	m_usb_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_usb_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_usb_table->setContextMenuPolicy(Qt::CustomContextMenu);
	m_usb_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_usb_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_usb_table->verticalScrollBar()->setSingleStep(20);
	m_usb_table->horizontalScrollBar()->setSingleStep(10);
	m_usb_table->setColumnCount(5);
	m_usb_table->setHorizontalHeaderLabels(QStringList() << tr("Device") << tr("Path") << tr("Vendor ID") << tr("Product ID") << tr("Serial"));
	m_usb_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
	m_usb_table->horizontalHeader()->setStretchLastSection(true);
	m_usb_table->setRowCount(max_usb_devices);

	for (int i = 0; i < max_usb_devices; i++)
	{
		const QString device_name = get_device_name(i);
		const cfg::device_info info = get_device_info(device_name, m_cfg_node->get_map());

		m_usb_table->setItem(i, usb_column::usb_name, new QTableWidgetItem(device_name));
		m_usb_table->setItem(i, usb_column::usb_path, new QTableWidgetItem(QString::fromStdString(info.path)));
		m_usb_table->setItem(i, usb_column::usb_vid, new QTableWidgetItem(QString::fromStdString(info.vid)));
		m_usb_table->setItem(i, usb_column::usb_pid, new QTableWidgetItem(QString::fromStdString(info.pid)));
		m_usb_table->setItem(i, usb_column::usb_serial, new QTableWidgetItem(QString::fromStdString(info.serial)));
	}

	m_usb_table->resizeColumnsToContents();

	connect(m_usb_table, &QTableWidget::customContextMenuRequested, this, &vfs_dialog_usb_tab::show_context_menu);
	connect(m_usb_table, &QTableWidget::itemDoubleClicked, this, &vfs_dialog_usb_tab::double_clicked_slot);

	QVBoxLayout* vbox = new QVBoxLayout;
	vbox->addWidget(m_usb_table);

	setLayout(vbox);
}

void vfs_dialog_usb_tab::set_settings() const
{
	cfg::map_of_type<cfg::device_info> device_map{};

	for (int i = 0; i < max_usb_devices; i++)
	{
		cfg::device_info info{};

		info.path   = m_usb_table->item(i, usb_column::usb_path)->text().toStdString();
		info.vid    = m_usb_table->item(i, usb_column::usb_vid)->text().toStdString();
		info.pid    = m_usb_table->item(i, usb_column::usb_pid)->text().toStdString();
		info.serial = m_usb_table->item(i, usb_column::usb_serial)->text().toStdString();

		device_map.emplace(get_device_name(i).toStdString(), std::move(info));
	}

	m_cfg_node->set_map(std::move(device_map));
}

void vfs_dialog_usb_tab::reset() const
{
	for (int i = 0; i < max_usb_devices; i++)
	{
		const QString device_name = get_device_name(i);
		const cfg::device_info info = get_device_info(device_name, m_cfg_node->get_default());

		m_usb_table->item(i, usb_column::usb_path)->setText(QString::fromStdString(info.path));
		m_usb_table->item(i, usb_column::usb_vid)->setText(QString::fromStdString(info.vid));
		m_usb_table->item(i, usb_column::usb_pid)->setText(QString::fromStdString(info.pid));
		m_usb_table->item(i, usb_column::usb_serial)->setText(QString::fromStdString(info.serial));
	}
}

void vfs_dialog_usb_tab::show_usb_input_dialog(int index)
{
	if (index < 0 || index >= max_usb_devices)
	{
		return;
	}

	const QString device_name = get_device_name(index);
	const cfg::device_info default_info = get_device_info(device_name, m_cfg_node->get_default());
	cfg::device_info info{};

	info.path   = m_usb_table->item(index, usb_column::usb_path)->text().toStdString();
	info.vid    = m_usb_table->item(index, usb_column::usb_vid)->text().toStdString();
	info.pid    = m_usb_table->item(index, usb_column::usb_pid)->text().toStdString();
	info.serial = m_usb_table->item(index, usb_column::usb_serial)->text().toStdString();

	vfs_dialog_usb_input* input_dialog = new vfs_dialog_usb_input(device_name, default_info, &info, m_gui_settings, this);
	if (input_dialog->exec() == QDialog::Accepted)
	{
		m_usb_table->item(index, usb_column::usb_path)->setText(QString::fromStdString(info.path));
		m_usb_table->item(index, usb_column::usb_vid)->setText(QString::fromStdString(info.vid));
		m_usb_table->item(index, usb_column::usb_pid)->setText(QString::fromStdString(info.pid));
		m_usb_table->item(index, usb_column::usb_serial)->setText(QString::fromStdString(info.serial));
	}
	input_dialog->deleteLater();
}

void vfs_dialog_usb_tab::show_context_menu(const QPoint& pos)
{
	const int row = m_usb_table->indexAt(pos).row();

	if (row < 0 || row >= max_usb_devices)
	{
		return;
	}

	QMenu menu{};
	QAction* edit = menu.addAction(tr("&Edit"));

	connect(edit, &QAction::triggered, this, [this, row]()
	{
		show_usb_input_dialog(row);
	});

	menu.exec(m_usb_table->viewport()->mapToGlobal(pos));
}

void vfs_dialog_usb_tab::double_clicked_slot(QTableWidgetItem* item)
{
	if (!item)
	{
		return;
	}

	show_usb_input_dialog(item->row());
}

void vfs_dialog_usb_tab::mouseDoubleClickEvent(QMouseEvent* ev)
{
	if (!ev) return;

	// Qt's itemDoubleClicked signal doesn't distinguish between mouse buttons and there is no simple way to get the pressed button.
	// So we have to ignore this event when another button is pressed.
	if (ev->button() != Qt::LeftButton)
	{
		ev->ignore();
		return;
	}

	QWidget::mouseDoubleClickEvent(ev);
}
