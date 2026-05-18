#include "stdafx.h"
#include "Utilities/File.h"
#include "skylander_dialog.h"
#include "Emu/Io/Skylander.h"

#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QComboBox>
#include <QPushButton>
#include <QStringList>
#include <QCompleter>

skylander_dialog* skylander_dialog::inst = nullptr;
QString last_skylander_path;

u16 skylander_crc16(u16 init_value, const u8* buffer, u32 size)
{
	constexpr unsigned short CRC_CCITT_TABLE[256] = {
	    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 0x1231, 0x0210, 0x3273,
	    0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528,
	    0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D, 0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC, 0x48C4, 0x58E5, 0x6886,
	    0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC, 0xFBBF,
	    0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 0x7E97, 0x6EB6, 0x5ED5,
	    0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78, 0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 0x1080, 0x00A1, 0x30C2,
	    0x20E3, 0x5004, 0x4025, 0x7046, 0x6067, 0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 0xB5EA, 0xA5CB, 0x95A8,
	    0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691,
	    0x16B0, 0x6657, 0x7676, 0x4615, 0x5634, 0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3, 0xCB7D, 0xDB5C, 0xEB3F,
	    0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07, 0x5C64,
	    0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0};

	u16 crc = init_value;

	for (u32 i = 0; i < size; i++)
	{
		const u16 tmp = (crc >> 8) ^ buffer[i];
		crc = (crc << 8) ^ CRC_CCITT_TABLE[tmp];
	}

	return crc;
}

skylander_creator_dialog::skylander_creator_dialog(QWidget* parent)
    : QDialog(parent)
{
	setWindowTitle(tr("Skylander Creator"));
	setObjectName("skylanders_creator");
	setMinimumSize(QSize(500, 150));

	QVBoxLayout* vbox_panel = new QVBoxLayout();

	QComboBox* combo_skylist = new QComboBox();
	QStringList filterlist;
	for (const auto& [entry, figure_name] : list_skylanders)
	{
		const uint qvar = (entry.first << 16) | entry.second;
		QString name = QString::fromStdString(figure_name);
		combo_skylist->addItem(name, QVariant(qvar));
		filterlist << std::move(name);
	}
	combo_skylist->addItem(tr("--Unknown--"), QVariant(0xFFFFFFFF));
	combo_skylist->setEditable(true);
	combo_skylist->setInsertPolicy(QComboBox::NoInsert);
	combo_skylist->model()->sort(0, Qt::AscendingOrder);

	QCompleter* co_compl = new QCompleter(filterlist, this);
	co_compl->setCaseSensitivity(Qt::CaseInsensitive);
	co_compl->setCompletionMode(QCompleter::PopupCompletion);
	co_compl->setFilterMode(Qt::MatchContains);
	combo_skylist->setCompleter(co_compl);

	vbox_panel->addWidget(combo_skylist);

	QFrame* line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	vbox_panel->addWidget(line);

	QHBoxLayout* hbox_idvar          = new QHBoxLayout();
	QLabel* label_id                 = new QLabel(tr("ID:"));
	QLabel* label_var                = new QLabel(tr("Variant:"));
	QLineEdit* edit_id               = new QLineEdit("0");
	QLineEdit* edit_var              = new QLineEdit("0");
	QRegularExpressionValidator* rxv = new QRegularExpressionValidator(QRegularExpression("\\d*"), this);
	edit_id->setValidator(rxv);
	edit_var->setValidator(rxv);
	hbox_idvar->addWidget(label_id);
	hbox_idvar->addWidget(edit_id);
	hbox_idvar->addWidget(label_var);
	hbox_idvar->addWidget(edit_var);
	vbox_panel->addLayout(hbox_idvar);

	QHBoxLayout* hbox_buttons = new QHBoxLayout();
	QPushButton* btn_create   = new QPushButton(tr("Create"), this);
	QPushButton* btn_cancel   = new QPushButton(tr("Cancel"), this);
	hbox_buttons->addStretch();
	hbox_buttons->addWidget(btn_create);
	hbox_buttons->addWidget(btn_cancel);
	vbox_panel->addLayout(hbox_buttons);

	setLayout(vbox_panel);

	connect(combo_skylist, &QComboBox::currentIndexChanged, [=](int index)
	{
		const u32 sky_info = combo_skylist->itemData(index).toUInt();
		if (sky_info != 0xFFFFFFFF)
		{
			const u16 sky_id  = sky_info >> 16;
			const u16 sky_var = sky_info & 0xFFFF;

			edit_id->setText(QString::number(sky_id));
			edit_var->setText(QString::number(sky_var));
		}
	});

	connect(btn_create, &QAbstractButton::clicked, this, [=, this]()
	{
		bool ok_id = false, ok_var = false;
		const u16 sky_id = edit_id->text().toUShort(&ok_id);
		if (!ok_id)
		{
			QMessageBox::warning(this, tr("Error converting value"), tr("ID entered is invalid!"), QMessageBox::Ok);
			return;
		}
		const u16 sky_var = edit_var->text().toUShort(&ok_var);
		if (!ok_var)
		{
			QMessageBox::warning(this, tr("Error converting value"), tr("Variant entered is invalid!"), QMessageBox::Ok);
			return;
		}

		QString predef_name = last_skylander_path;
		const auto found_sky = list_skylanders.find(std::make_pair(sky_id, sky_var));
		if (found_sky != list_skylanders.cend())
		{
			predef_name += QString::fromStdString(found_sky->second + ".sky");
		}
		else
		{
			predef_name += QString("Unknown(%1 %2).sky").arg(sky_id).arg(sky_var);
		}

		file_path = QFileDialog::getSaveFileName(this, tr("Create Skylander File"), predef_name, tr("Skylander Object (*.sky);;All Files (*)"));
		if (file_path.isEmpty())
		{
			return;
		}

		fs::file sky_file(file_path.toStdString(), fs::read + fs::write + fs::create);
		if (!sky_file)
		{
			QMessageBox::warning(this, tr("Failed to create skylander file!"), tr("Failed to create skylander file:\n%1").arg(file_path), QMessageBox::Ok);
			return;
		}

		std::array<u8, 0x40 * 0x10> buf{};
		const auto data = buf.data();
		// Set the block permissions
		write_to_ptr<le_t<u32>>(data, 0x36, 0x690F0F0F);
		for (u32 index = 1; index < 0x10; index++)
		{
			write_to_ptr<le_t<u32>>(data, (index * 0x40) + 0x36, 0x69080F7F);
		}
		// Set the skylander infos
		write_to_ptr<le_t<u16>>(data, (sky_id | sky_var) + 1);
		write_to_ptr<le_t<u16>>(data, 0x10, sky_id);
		write_to_ptr<le_t<u16>>(data, 0x1C, sky_var);
		// Set checksum
		write_to_ptr<le_t<u16>>(data, 0x1E, skylander_crc16(0xFFFF, data, 0x1E));

		sky_file.write(buf.data(), buf.size());
		sky_file.close();

		last_skylander_path = QFileInfo(file_path).absolutePath() + "/";
		accept();
	});

	connect(btn_cancel, &QAbstractButton::clicked, this, &QDialog::reject);

	connect(co_compl, qOverload<const QString&>(&QCompleter::activated), [=](const QString& text)
	{
		combo_skylist->setCurrentText(text);
		combo_skylist->setCurrentIndex(combo_skylist->findText(text));
	});
}

QString skylander_creator_dialog::get_file_path() const
{
	return file_path;
}

skylander_dialog::skylander_dialog(QWidget* parent)
    : QDialog(parent)
{
	setWindowTitle(tr("Skylanders Manager"));
	setObjectName("skylanders_manager");
	setAttribute(Qt::WA_DeleteOnClose);
	setMinimumSize(QSize(700, 200));

	QVBoxLayout* vbox_panel = new QVBoxLayout();

	auto add_line = [](QVBoxLayout* vbox)
	{
		QFrame* line = new QFrame();
		line->setFrameShape(QFrame::HLine);
		line->setFrameShadow(QFrame::Sunken);
		vbox->addWidget(line);
	};

	QGroupBox* group_skylanders = new QGroupBox(tr("Active Portal Skylanders:"));
	QVBoxLayout* vbox_group     = new QVBoxLayout();

	for (auto i = 0; i < UI_SKY_NUM; i++)
	{
		if (i != 0)
		{
			add_line(vbox_group);
		}

		QHBoxLayout* hbox_skylander = new QHBoxLayout();
		QLabel* label_skyname       = new QLabel(QString(tr("Skylander %1")).arg(i + 1));
		edit_skylanders[i]          = new QLineEdit();
		edit_skylanders[i]->setEnabled(false);

		QPushButton* clear_btn  = new QPushButton(tr("Clear"));
		QPushButton* create_btn = new QPushButton(tr("Create"));
		QPushButton* load_btn   = new QPushButton(tr("Load"));

		connect(clear_btn, &QAbstractButton::clicked, this, [this, i]() { clear_skylander(i); });
		connect(create_btn, &QAbstractButton::clicked, this, [this, i]() { create_skylander(i); });
		connect(load_btn, &QAbstractButton::clicked, this, [this, i]() { load_skylander(i); });

		hbox_skylander->addWidget(label_skyname);
		hbox_skylander->addWidget(edit_skylanders[i]);
		hbox_skylander->addWidget(clear_btn);
		hbox_skylander->addWidget(create_btn);
		hbox_skylander->addWidget(load_btn);

		vbox_group->addLayout(hbox_skylander);
	}

	group_skylanders->setLayout(vbox_group);
	vbox_panel->addWidget(group_skylanders);
	setLayout(vbox_panel);

	update_edits();
}

skylander_dialog::~skylander_dialog()
{
	inst = nullptr;
}

skylander_dialog* skylander_dialog::get_dlg(QWidget* parent)
{
	if (inst == nullptr) {
		inst = new skylander_dialog(parent);
	} else {
		inst->update_edits();
	}

	return inst;
}

void skylander_dialog::clear_skylander(u8 slot)
{
	if (const auto& slot_infos = g_skyportal.get_skylander(slot))
	{
		const auto& [cur_slot, id, var] = slot_infos.value();
		g_skyportal.remove_skylander(cur_slot);
		update_edits();
	}
}

void skylander_dialog::create_skylander(u8 slot)
{
	skylander_creator_dialog create_dlg(this);
	if (create_dlg.exec() == Accepted)
	{
		load_skylander_path(slot, create_dlg.get_file_path());
	}
}

void skylander_dialog::load_skylander(u8 slot)
{
	const QString file_path = QFileDialog::getOpenFileName(this, tr("Select Skylander File"), last_skylander_path, tr("Skylander (*.sky *.bin *.dmp *.dump);;All Files (*)"));
	if (file_path.isEmpty())
	{
		return;
	}

	last_skylander_path = QFileInfo(file_path).absolutePath() + "/";

	load_skylander_path(slot, file_path);
}

void skylander_dialog::load_skylander_path(u8 slot, const QString& path)
{
	fs::file sky_file(path.toStdString(), fs::read + fs::write + fs::lock);
	if (!sky_file)
	{
		QMessageBox::warning(this, tr("Failed to open the skylander file!"), tr("Failed to open the skylander file(%1)!\nFile may already be in use on the portal.").arg(path), QMessageBox::Ok);
		return;
	}

	std::array<u8, 0x40 * 0x10> data;
	if (sky_file.read(data.data(), data.size()) != data.size())
	{
		QMessageBox::warning(this, tr("Failed to read the skylander file!"), tr("Failed to read the skylander file(%1)!\nFile was too small.").arg(path), QMessageBox::Ok);
		return;
	}

	clear_skylander(slot);

	g_skyportal.load_skylander(slot, data.data(), std::move(sky_file));

	update_edits();
}

void skylander_dialog::update_edits()
{
	for (auto i = 0; i < UI_SKY_NUM; i++)
	{
		QString display_string;
		if (const auto& sd = g_skyportal.get_skylander(i))
		{
			const auto& [portal_slot, sky_id, sky_var] = sd.value();
			const auto found_sky = list_skylanders.find(std::make_pair(sky_id, sky_var));
			if (found_sky != list_skylanders.cend())
			{
				display_string = QString::fromStdString(found_sky->second);
			}
			else
			{
				display_string = QString(tr("Unknown (Id:%1 Var:%2)")).arg(sky_id).arg(sky_var);
			}
		}
		else
		{
			display_string = tr("None");
		}

		edit_skylanders[i]->setText(display_string);
	}
}
