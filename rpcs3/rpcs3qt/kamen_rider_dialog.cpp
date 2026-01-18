#include "stdafx.h"
#include "Utilities/File.h"
#include "kamen_rider_dialog.h"
#include "Emu/Io/KamenRider.h"

#include <random>

#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QComboBox>
#include <QPushButton>
#include <QStringList>
#include <QCompleter>

kamen_rider_dialog* kamen_rider_dialog::inst = nullptr;
std::array<std::optional<std::tuple<u8, u8, u8, u8>>, UI_FIG_NUM> kamen_rider_dialog::figure_slots = {};
QString last_kamen_rider_path;

static const std::map<const std::tuple<const u8, const u8, const u8>, const std::string> list_kamen_riders = {
	// Character ID [0x1b], ERC type [0x1a], Figure type [0x19]
	{{0x10, 0x01, 0x10}, "Kamen Rider Drive (Wind)"},
	{{0x10, 0x01, 0x20}, "Kamen Rider Drive (Water)"},
	{{0x10, 0x01, 0x30}, "Kamen Rider Drive (Fire)"},
	{{0x10, 0x01, 0x40}, "Kamen Rider Drive (Light)"},
	{{0x10, 0x01, 0x50}, "Kamen Rider Drive (Dark)"},
		{{0x20, 0x01, 0x00}, "Kamen Rider Drive - Type Wild"},
		{{0x20, 0x02, 0x00}, "Kamen Rider Drive - Type Wild Gyasha Ver"},
	 // {{    ,     ,     }, "Kamen Rider Drive - Type Speed Flare"},
	 // {{    ,     ,     }, "Kamen Rider Drive - Type Technic"}, // 1.05 update
	{{0x11, 0x01, 0x10}, "Kamen Rider Gaim (Wind)"},
	{{0x11, 0x01, 0x20}, "Kamen Rider Gaim (Water)"},
		{{0x21, 0x01, 0x00}, "Kamen Rider Gaim - Jimber Lemon Arms"},
		{{0x21, 0x02, 0x00}, "Kamen Rider Gaim - Kachidoki Arms"},
		{{0x21, 0x03, 0x00}, "Kamen Rider Gaim - Kiwami Arms"},
	{{0x12, 0x01, 0x20}, "Kamen Rider Wizard (Water)"},
	{{0x12, 0x01, 0x30}, "Kamen Rider Wizard (Fire)"},
		{{0x22, 0x01, 0x00}, "Kamen Rider Wizard - Infinity Style"},
		{{0x22, 0x02, 0x00}, "Kamen Rider Wizard - All Dragon"},
		{{0x22, 0x03, 0x00}, "Kamen Rider Wizard - Infinity Gold Dragon"},
	{{0x13, 0x01, 0x40}, "Kamen Rider Fourze (Light)"},
		{{0x23, 0x01, 0x00}, "Kamen Rider Fourze - Magnet States"},
		{{0x23, 0x02, 0x00}, "Kamen Rider Fourze - Cosmic States"},
		{{0x23, 0x03, 0x00}, "Kamen Rider Fourze - Meteor Nadeshiko Fusion States"},
	{{0x14, 0x01, 0x20}, "Kamen Rider OOO (Water)"},
		{{0x24, 0x01, 0x00}, "Kamen Rider OOO - Super Tatoba Combo"},
		{{0x24, 0x02, 0x00}, "Kamen Rider OOO - Putotyra Combo"},
		{{0x24, 0x04, 0x00}, "Kamen Rider OOO - Tajadol Combo"},
	{{0x15, 0x01, 0x10}, "Kamen Rider W (Double) (Wind)"},
		{{0x25, 0x01, 0x00}, "Kamen Rider W (Double) - Cyclone Joker Extreme"},
		{{0x25, 0x02, 0x00}, "Kamen Rider W (Double) - Cyclone Joker Gold Extreme"},
		{{0x25, 0x03, 0x00}, "Kamen Rider W (Double) - Fang Joker"},
	{{0x16, 0x01, 0x50}, "Kamen Rider Decade (Dark)"},
		{{0x26, 0x01, 0x00}, "Kamen Rider Decade - Complete Form"},
		{{0x26, 0x02, 0x00}, "Kamen Rider Decade - Strongest Complete Form"},
		{{0x26, 0x03, 0x00}, "Kamen Rider Decade - Final Form"},
	{{0x17, 0x01, 0x50}, "Kamen Rider Kiva (Dark)"},
		{{0x27, 0x01, 0x00}, "Kamen Rider Kiva - Dogabaki Form"},
		{{0x27, 0x02, 0x00}, "Kamen Rider Kiva - Emperor Form"},
	{{0x18, 0x01, 0x40}, "Kamen Rider Den-O (Light)"},
		{{0x28, 0x01, 0x00}, "Kamen Rider Den-O - Super Climax Form"},
		{{0x28, 0x02, 0x00}, "Kamen Rider Den-O - Liner Form"},
		{{0x28, 0x03, 0x00}, "Kamen Rider Den-O - Climax Form"},
	{{0x19, 0x01, 0x30}, "Kamen Rider Kabuto (Fire)"},
		{{0x29, 0x01, 0x00}, "Kamen Rider Kabuto - Hyper Form"},
		{{0x29, 0x02, 0x00}, "Kamen Rider Kabuto - Masked Form"},
	{{0x1a, 0x01, 0x30}, "Kamen Rider Hibiki (Fire)"},
		{{0x2a, 0x01, 0x00}, "Kamen Rider Hibiki - Kurenai"},
		{{0x2a, 0x02, 0x00}, "Kamen Rider Hibiki - Armed"},
	{{0x1b, 0x01, 0x50}, "Kamen Rider Blade (Dark)"},
		{{0x2b, 0x01, 0x00}, "Kamen Rider Blade - Joker Form"},
		{{0x2b, 0x02, 0x00}, "Kamen Rider Blade - King Form"},
	{{0x1c, 0x01, 0x50}, "Kamen Rider Faiz (Dark)"},
		{{0x2c, 0x01, 0x00}, "Kamen Rider Faiz - Axel Form"},
		{{0x2c, 0x02, 0x00}, "Kamen Rider Faiz - Blaster Form"},
	{{0x1d, 0x01, 0x10}, "Kamen Rider Ryuki (Wind)"},
		{{0x2d, 0x01, 0x00}, "Kamen Rider Ryuki - Dragreder"},
		{{0x2d, 0x02, 0x00}, "Kamen Rider Ryuki - Survive"},
	{{0x1e, 0x01, 0x20}, "Kamen Rider Agito (Water)"},
		{{0x2e, 0x01, 0x00}, "Kamen Rider Agito - Shining Form"},
		{{0x2e, 0x02, 0x00}, "Kamen Rider Agito - Burning Form"},
	{{0x1f, 0x01, 0x40}, "Kamen Rider Kuuga (Light)"},
		{{0x2f, 0x01, 0x00}, "Kamen Rider Kuuga - Ultimate Form"},
		{{0x2f, 0x02, 0x00}, "Kamen Rider Kuuga - Amazing Mighty"},

	{{0x31, 0x01, 0x00}, "Kamen Rider Baron"},
	{{0x31, 0x02, 0x00}, "Kamen Rider Zangetsu Shin"},
	{{0x32, 0x01, 0x00}, "Kamen Rider Beast"},
	{{0x33, 0x01, 0x00}, "Kamen Rider Meteor"},
	{{0x34, 0x01, 0x00}, "Kamen Rider Birth"},
	{{0x35, 0x01, 0x00}, "Kamen Rider Accel"},
	{{0x36, 0x01, 0x00}, "Kamen Rider Diend"},
	{{0x36, 0x02, 0x00}, "Kamen Rider Shocker Combatman"},
	{{0x39, 0x01, 0x00}, "Kamen Rider Gatack"},
	// {{    ,     ,     }, "Kamen Rider Mach"}, // 01.05 update
};

static u32 kamen_rider_crc32(const std::array<u8, 16>& buffer)
{
	static constexpr std::array<u32, 256> CRC32_TABLE{
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535,
		0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd,
		0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d,
		0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
		0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
		0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
		0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac,
		0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
		0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab,
		0xb6662d3d, 0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
		0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb,
		0x086d3d2d, 0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
		0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea,
		0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 0x4db26158, 0x3ab551ce,
		0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
		0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
		0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409,
		0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
		0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739,
		0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
		0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344, 0x8708a3d2, 0x1e01f268,
		0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0,
		0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8,
		0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
		0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703,
		0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7,
		0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
		0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae,
		0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
		0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 0x88085ae6,
		0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
		0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d,
		0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5,
		0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
		0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
		0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

	// Kamen Rider figures calculate their CRC32 based on 12 bytes in the block of 16
	u32 ret = 0;
	for (u32 i = 0; i < 12; ++i)
	{
		const u8 index = u8(ret & 0xFF) ^ buffer[i];
		ret = ((ret >> 8) ^ CRC32_TABLE[index]);
	}

	return ret;
}

kamen_rider_creator_dialog::kamen_rider_creator_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Kamen Rider Creator"));
	setObjectName("kamen_rider_creator");
	setMinimumSize(QSize(500, 150));

	QVBoxLayout* vbox_panel = new QVBoxLayout();

	QComboBox* combo_figlist = new QComboBox();
	QStringList filterlist;
	for (const auto& [entry, figure_name] : list_kamen_riders)
	{
		const auto& [character_id, erc_type, figure_type] = entry;
		const uint qvar = (character_id << 16) | (erc_type << 8) | figure_type;
		QString name = QString::fromStdString(figure_name);
		combo_figlist->addItem(name, QVariant(qvar));
		filterlist << std::move(name);
	}
	combo_figlist->addItem(tr("--Unknown--"), QVariant(0xFFFFFFFF));
	combo_figlist->setEditable(true);
	combo_figlist->setInsertPolicy(QComboBox::NoInsert);
	combo_figlist->model()->sort(0, Qt::AscendingOrder);

	QCompleter* co_compl = new QCompleter(filterlist, this);
	co_compl->setCaseSensitivity(Qt::CaseInsensitive);
	co_compl->setCompletionMode(QCompleter::PopupCompletion);
	co_compl->setFilterMode(Qt::MatchContains);
	combo_figlist->setCompleter(co_compl);

	vbox_panel->addWidget(combo_figlist);

	QFrame* line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	vbox_panel->addWidget(line);

	QHBoxLayout* hbox_idvar = new QHBoxLayout();
	QLabel* label_id = new QLabel(tr("Character:"));
	QLabel* label_erc = new QLabel(tr("ERC:"));
	QLabel* label_fig = new QLabel(tr("Figure:"));
	QLineEdit* edit_id = new QLineEdit("0");
	QLineEdit* edit_erc = new QLineEdit("0");
	QLineEdit* edit_fig = new QLineEdit("0");
	QRegularExpressionValidator* rxv = new QRegularExpressionValidator(QRegularExpression("\\d*"), this);
	edit_id->setValidator(rxv);
	edit_erc->setValidator(rxv);
	edit_fig->setValidator(rxv);
	hbox_idvar->addWidget(label_id);
	hbox_idvar->addWidget(edit_id);
	hbox_idvar->addWidget(label_erc);
	hbox_idvar->addWidget(edit_erc);
	hbox_idvar->addWidget(label_fig);
	hbox_idvar->addWidget(edit_fig);
	vbox_panel->addLayout(hbox_idvar);

	QHBoxLayout* hbox_buttons = new QHBoxLayout();
	QPushButton* btn_create = new QPushButton(tr("Create"), this);
	QPushButton* btn_cancel = new QPushButton(tr("Cancel"), this);
	hbox_buttons->addStretch();
	hbox_buttons->addWidget(btn_create);
	hbox_buttons->addWidget(btn_cancel);
	vbox_panel->addLayout(hbox_buttons);

	setLayout(vbox_panel);

	connect(combo_figlist, &QComboBox::currentIndexChanged, [=](int index)
		{
			const u32 fig_info = combo_figlist->itemData(index).toUInt();
			if (fig_info != 0xFFFFFFFF)
			{
				const u8 character_id = (fig_info >> 16) & 0xff;
				const u8 erc_type     = (fig_info >>  8) & 0xff;
				const u8 figure_type  =  fig_info        & 0xff;

				edit_id->setText(QString::number(character_id));
				edit_erc->setText(QString::number(erc_type));
				edit_fig->setText(QString::number(figure_type));
			}
		});

	connect(btn_create, &QAbstractButton::clicked, this, [=, this]()
		{
			bool ok_character = false, ok_erc = false, ok_fig = false;
			const u8 character_id = edit_id->text().toUShort(&ok_character);
			if (!ok_character)
			{
				QMessageBox::warning(this, tr("Error converting value"), tr("ID entered is invalid!"), QMessageBox::Ok);
				return;
			}
			const u8 erc_type = edit_erc->text().toUShort(&ok_erc);
			if (!ok_erc)
			{
				QMessageBox::warning(this, tr("Error converting value"), tr("ERC entered is invalid!"), QMessageBox::Ok);
				return;
			}
			const u8 figure_type = edit_fig->text().toUShort(&ok_fig);
			if (!ok_fig)
			{
				QMessageBox::warning(this, tr("Error converting value"), tr("Figure entered is invalid!"), QMessageBox::Ok);
				return;
			}

			QString predef_name = last_kamen_rider_path;
			const auto found_fig = list_kamen_riders.find(std::make_tuple(character_id, erc_type, figure_type));
			if (found_fig != list_kamen_riders.cend())
			{
				predef_name += QString::fromStdString(found_fig->second + ".bin");
			}
			else
			{
				predef_name += QString("Unknown(%1 %2 %3).bin").arg(character_id).arg(erc_type).arg(figure_type);
			}

			file_path = QFileDialog::getSaveFileName(this, tr("Create Kamen Rider File"), predef_name, tr("Kamen Rider Object (*.bin);;All Files (*)"));
			if (file_path.isEmpty())
			{
				return;
			}

			fs::file fig_file(file_path.toStdString(), fs::read + fs::write + fs::create);
			if (!fig_file)
			{
				QMessageBox::warning(this, tr("Failed to create kamen rider file!"), tr("Failed to create kamen rider file:\n%1").arg(file_path), QMessageBox::Ok);
				return;
			}

			std::array<u8, 0x14 * 0x10> buf{};

			buf[0] = 0x04;
			buf[6] = 0x80;

			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_int_distribution<int> dist(0, 255);

			buf[1] = dist(mt);
			buf[2] = dist(mt);
			buf[3] = dist(mt);
			buf[4] = dist(mt);
			buf[5] = dist(mt);

			buf[7] = 0x89;
			buf[8] = 0x44;
			buf[10] = 0xc2;
			std::array<u8, 16> figure_data = {u8(dist(mt)), 0x03, 0x00, 0x00, 0x01, 0x0e, 0x0a, 0x0a, 0x10, figure_type, erc_type, character_id};
			write_to_ptr<le_t<u32>>(figure_data.data(), 0xC, kamen_rider_crc32(figure_data));
			memcpy(&buf[16], figure_data.data(), figure_data.size());
			fig_file.write(buf.data(), buf.size());
			fig_file.close();

			last_kamen_rider_path = QFileInfo(file_path).absolutePath() + "/";
			accept();
		});

	connect(btn_cancel, &QAbstractButton::clicked, this, &QDialog::reject);

	connect(co_compl, qOverload<const QString&>(&QCompleter::activated), [=](const QString& text)
		{
			combo_figlist->setCurrentText(text);
			combo_figlist->setCurrentIndex(combo_figlist->findText(text));
		});
}

QString kamen_rider_creator_dialog::get_file_path() const
{
	return file_path;
}

kamen_rider_dialog::kamen_rider_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Kamen Rider Manager"));
	setObjectName("kamen_riders_manager");
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

	QGroupBox* group_kamen_riders = new QGroupBox(tr("Active Kamen Riders:"));
	QVBoxLayout* vbox_group = new QVBoxLayout();

	for (auto i = 0; i < UI_FIG_NUM; i++)
	{
		if (i != 0)
		{
			add_line(vbox_group);
		}

		QHBoxLayout* hbox_kamen_rider = new QHBoxLayout();
		QLabel* label_figname = new QLabel(QString(tr("Kamen Rider %1")).arg(i + 1));
		edit_kamen_riders[i] = new QLineEdit();
		edit_kamen_riders[i]->setEnabled(false);

		QPushButton* clear_btn = new QPushButton(tr("Clear"));
		QPushButton* create_btn = new QPushButton(tr("Create"));
		QPushButton* load_btn = new QPushButton(tr("Load"));

		connect(clear_btn, &QAbstractButton::clicked, this, [this, i]()
			{
				clear_kamen_rider(i);
			});
		connect(create_btn, &QAbstractButton::clicked, this, [this, i]()
			{
				create_kamen_rider(i);
			});
		connect(load_btn, &QAbstractButton::clicked, this, [this, i]()
			{
				load_kamen_rider(i);
			});

		hbox_kamen_rider->addWidget(label_figname);
		hbox_kamen_rider->addWidget(edit_kamen_riders[i]);
		hbox_kamen_rider->addWidget(clear_btn);
		hbox_kamen_rider->addWidget(create_btn);
		hbox_kamen_rider->addWidget(load_btn);

		vbox_group->addLayout(hbox_kamen_rider);
	}

	group_kamen_riders->setLayout(vbox_group);
	vbox_panel->addWidget(group_kamen_riders);
	setLayout(vbox_panel);

	update_edits();
}

kamen_rider_dialog::~kamen_rider_dialog()
{
	inst = nullptr;
}

kamen_rider_dialog* kamen_rider_dialog::get_dlg(QWidget* parent)
{
	if (inst == nullptr)
		inst = new kamen_rider_dialog(parent);

	return inst;
}

void kamen_rider_dialog::clear_kamen_rider(u8 slot)
{
	if (const auto& slot_infos = ::at32(figure_slots, slot))
	{
		const auto& [cur_slot, character_id, erc_type, figure_type] = slot_infos.value();
		g_ridergate.remove_figure(cur_slot);
		figure_slots[slot] = {};
		update_edits();
	}
}

void kamen_rider_dialog::create_kamen_rider(u8 slot)
{
	kamen_rider_creator_dialog create_dlg(this);
	if (create_dlg.exec() == Accepted)
	{
		load_kamen_rider_path(slot, create_dlg.get_file_path());
	}
}

void kamen_rider_dialog::load_kamen_rider(u8 slot)
{
	const QString file_path = QFileDialog::getOpenFileName(this, tr("Select Kamen Rider File"), last_kamen_rider_path, tr("Kamen Rider (*.bin);;All Files (*)"));
	if (file_path.isEmpty())
	{
		return;
	}

	last_kamen_rider_path = QFileInfo(file_path).absolutePath() + "/";

	load_kamen_rider_path(slot, file_path);
}

void kamen_rider_dialog::load_kamen_rider_path(u8 slot, const QString& path)
{
	fs::file fig_file(path.toStdString(), fs::read + fs::write + fs::lock);
	if (!fig_file)
	{
		QMessageBox::warning(this, tr("Failed to open the kamen rider file!"), tr("Failed to open the kamen rider file(%1)!\nFile may already be in use on the portal.").arg(path), QMessageBox::Ok);
		return;
	}

	std::array<u8, 0x14 * 0x10> data;
	if (fig_file.read(data.data(), data.size()) != data.size())
	{
		QMessageBox::warning(this, tr("Failed to read the kamen rider file!"), tr("Failed to read the kamen rider file(%1)!\nFile was too small.").arg(path), QMessageBox::Ok);
		return;
	}

	clear_kamen_rider(slot);

	u8 character_id = data[0x1b];
	u8 erc_type = data[0x1a];
	u8 figure_type = data[0x19];

	u8 portal_slot = g_ridergate.load_figure(data, std::move(fig_file));
	figure_slots[slot] = std::tuple(portal_slot, character_id, erc_type, figure_type);

	update_edits();
}

void kamen_rider_dialog::update_edits()
{
	for (auto i = 0; i < UI_FIG_NUM; i++)
	{
		QString display_string;
		if (const auto& sd = figure_slots[i])
		{
			const auto& [portal_slot, character_id, erc_type, figure_type] = sd.value();
			const auto found_fig = list_kamen_riders.find(std::make_tuple(character_id, erc_type, figure_type));
			if (found_fig != list_kamen_riders.cend())
			{
				display_string = QString::fromStdString(found_fig->second);
			}
			else
			{
				display_string = QString(tr("Unknown (Character:%1 ERC:%2 Figure:%3)")).arg(character_id).arg(erc_type).arg(figure_type);
			}
		}
		else
		{
			display_string = tr("None");
		}

		edit_kamen_riders[i]->setText(display_string);
	}
}
