#include "stdafx.h"
#include "Utilities/File.h"
#include "dimensions_dialog.h"
#include "Emu/Io/Dimensions.h"

#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QComboBox>
#include <QPushButton>
#include <QStringList>
#include <QCompleter>
#include <QGridLayout>

dimensions_dialog* dimensions_dialog::inst = nullptr;
std::array<std::optional<u32>, 7> figure_slots = {};
static QString s_last_figure_path;

LOG_CHANNEL(dimensions_log, "dimensions");

const std::map<const u32, const std::string> list_minifigs = {
	{0, "Blank Tag"},
	{1, "Batman"},
	{2, "Gandalf"},
	{3, "Wyldstyle"},
	{4, "Aquaman"},
	{5, "Bad Cop"},
	{6, "Bane"},
	{7, "Bart Simpson"},
	{8, "Benny"},
	{9, "Chell"},
	{10, "Cole"},
	{11, "Cragger"},
	{12, "Cyborg"},
	{13, "Cyberman"},
	{14, "Doc Brown"},
	{15, "The Doctor"},
	{16, "Emmet"},
	{17, "Eris"},
	{18, "Gimli"},
	{19, "Gollum"},
	{20, "Harley Quinn"},
	{21, "Homer Simpson"},
	{22, "Jay"},
	{23, "Joker"},
	{24, "Kai"},
	{25, "ACU Trooper"},
	{26, "Gamer Kid"},
	{27, "Krusty the Clown"},
	{28, "Laval"},
	{29, "Legolas"},
	{30, "Lloyd"},
	{31, "Marty McFly"},
	{32, "Nya"},
	{33, "Owen Grady"},
	{34, "Peter Venkman"},
	{35, "Slimer"},
	{36, "Scooby-Doo"},
	{37, "Sensei Wu"},
	{38, "Shaggy"},
	{39, "Stay Puft"},
	{40, "Superman"},
	{41, "Unikitty"},
	{42, "Wicked Witch of the West"},
	{43, "Wonder Woman"},
	{44, "Zane"},
	{45, "Green Arrow"},
	{46, "Supergirl"},
	{47, "Abby Yates"},
	{48, "Finn the Human"},
	{49, "Ethan Hunt"},
	{50, "Lumpy Space Princess"},
	{51, "Jake the Dog"},
	{52, "Harry Potter"},
	{53, "Lord Voldemort"},
	{54, "Michael Knight"},
	{55, "B.A. Baracus"},
	{56, "Newt Scamander"},
	{57, "Sonic the Hedgehog"},
	{58, "Future Update (unreleased)"},
	{59, "Gizmo"},
	{60, "Stripe"},
	{61, "E.T."},
	{62, "Tina Goldstein"},
	{63, "Marceline the Vampire Queen"},
	{64, "Batgirl"},
	{65, "Robin"},
	{66, "Sloth"},
	{67, "Hermione Granger"},
	{68, "Chase McCain"},
	{69, "Excalibur Batman"},
	{70, "Raven"},
	{71, "Beast Boy"},
	{72, "Betelgeuse"},
	{73, "Lord Vortech (unreleased)"},
	{74, "Blossom"},
	{75, "Bubbles"},
	{76, "Buttercup"},
	{77, "Starfire"},
	{78, "World 15 (unreleased)"},
	{79, "World 16 (unreleased)"},
	{80, "World 17 (unreleased)"},
	{81, "World 18 (unreleased)"},
	{82, "World 19 (unreleased)"},
	{83, "World 20 (unreleased)"},
	{768, "Unknown 768"},
	{769, "Supergirl Red Lantern"},
	{770, "Unknown 770"}};

const std::map<const u32, const std::string> list_tokens = {
	{1000, "Police Car"},
	{1001, "Aerial Squad Car"},
	{1002, "Missile Striker"},
	{1003, "Gravity Sprinter"},
	{1004, "Street Shredder"},
	{1005, "Sky Clobberer"},
	{1006, "Batmobile"},
	{1007, "Batblaster"},
	{1008, "Sonic Batray"},
	{1009, "Benny's Spaceship"},
	{1010, "Lasercraft"},
	{1011, "The Annihilator"},
	{1012, "DeLorean Time Machine"},
	{1013, "Electric Time Machine"},
	{1014, "Ultra Time Machine"},
	{1015, "Hoverboard"},
	{1016, "Cyclone Board"},
	{1017, "Ultimate Hoverjet"},
	{1018, "Eagle Interceptor"},
	{1019, "Eagle Sky Blazer"},
	{1020, "Eagle Swoop Diver"},
	{1021, "Swamp Skimmer"},
	{1022, "Cragger's Fireship"},
	{1023, "Croc Command Sub"},
	{1024, "Cyber-Guard"},
	{1025, "Cyber-Wrecker"},
	{1026, "Laser Robot Walker"},
	{1027, "K-9"},
	{1028, "K-9 Ruff Rover"},
	{1029, "K-9 Laser Cutter"},
	{1030, "TARDIS"},
	{1031, "Laser-Pulse TARDIS"},
	{1032, "Energy-Burst TARDIS"},
	{1033, "Emmet's Excavator"},
	{1034, "Destroy Dozer"},
	{1035, "Destruct-o-Mech"},
	{1036, "Winged Monkey"},
	{1037, "Battle Monkey"},
	{1038, "Commander Monkey"},
	{1039, "Axe Chariot"},
	{1040, "Axe Hurler"},
	{1041, "Soaring Chariot"},
	{1042, "Shelob the Great"},
	{1043, "8-Legged Stalker"},
	{1044, "Poison Slinger"},
	{1045, "Homer's Car"},
	{1047, "The SubmaHomer"},
	{1046, "The Homercraft"},
	{1048, "Taunt-o-Vision"},
	{1050, "The MechaHomer"},
	{1049, "Blast Cam"},
	{1051, "Velociraptor"},
	{1053, "Venom Raptor"},
	{1052, "Spike Attack Raptor"},
	{1054, "Gyrosphere"},
	{1055, "Sonic Beam Gyrosphere"},
	{1056, " Gyrosphere"},
	{1057, "Clown Bike"},
	{1058, "Cannon Bike"},
	{1059, "Anti-Gravity Rocket Bike"},
	{1060, "Mighty Lion Rider"},
	{1061, "Lion Blazer"},
	{1062, "Fire Lion"},
	{1063, "Arrow Launcher"},
	{1064, "Seeking Shooter"},
	{1065, "Triple Ballista"},
	{1066, "Mystery Machine"},
	{1067, "Mystery Tow & Go"},
	{1068, "Mystery Monster"},
	{1069, "Boulder Bomber"},
	{1070, "Boulder Blaster"},
	{1071, "Cyclone Jet"},
	{1072, "Storm Fighter"},
	{1073, "Lightning Jet"},
	{1074, "Electro-Shooter"},
	{1075, "Blade Bike"},
	{1076, "Flight Fire Bike"},
	{1077, "Blades of Fire"},
	{1078, "Samurai Mech"},
	{1079, "Samurai Shooter"},
	{1080, "Soaring Samurai Mech"},
	{1081, "Companion Cube"},
	{1082, "Laser Deflector"},
	{1083, "Gold Heart Emitter"},
	{1084, "Sentry Turret"},
	{1085, "Turret Striker"},
	{1086, "Flight Turret Carrier"},
	{1087, "Scooby Snack"},
	{1088, "Scooby Fire Snack"},
	{1089, "Scooby Ghost Snack"},
	{1090, "Cloud Cuckoo Car"},
	{1091, "X-Stream Soaker"},
	{1092, "Rainbow Cannon"},
	{1093, "Invisible Jet"},
	{1094, "Laser Shooter"},
	{1095, "Torpedo Bomber"},
	{1096, "NinjaCopter"},
	{1097, "Glaciator"},
	{1098, "Freeze Fighter"},
	{1099, "Travelling Time Train"},
	{1100, "Flight Time Train"},
	{1101, "Missile Blast Time Train"},
	{1102, "Aqua Watercraft"},
	{1103, "Seven Seas Speeder"},
	{1104, "Trident of Fire"},
	{1105, "Drill Driver"},
	{1106, "Bane Dig 'n' Drill"},
	{1107, "Bane Drill 'n' Blast"},
	{1108, "Quinn Mobile"},
	{1109, "Quinn Ultra Racer"},
	{1110, "Missile Launcher"},
	{1111, "The Joker's Chopper"},
	{1112, "Mischievous Missile Blaster"},
	{1113, "Lock 'n' Laser Jet"},
	{1114, "Hover Pod"},
	{1115, "Krypton Striker"},
	{1116, "Super Stealth Pod"},
	{1117, "Dalek"},
	{1118, "Fire 'n' Ride Dalek"},
	{1119, "Silver Shooter Dalek"},
	{1120, "Ecto-1"},
	{1121, "Ecto-1 Blaster"},
	{1122, "Ecto-1 Water Diver"},
	{1123, "Ghost Trap"},
	{1124, "Ghost Stun 'n' Trap"},
	{1125, "Proton Zapper"},
	{1126, "Unknown"},
	{1127, "Unknown"},
	{1128, "Unknown"},
	{1129, "Unknown"},
	{1130, "Unknown"},
	{1131, "Unknown"},
	{1132, "Lloyd's Golden Dragon"},
	{1133, "Sword Projector Dragon"},
	{1134, "Unknown"},
	{1135, "Unknown"},
	{1136, "Unknown"},
	{1137, "Unknown"},
	{1138, "Unknown"},
	{1139, "Unknown"},
	{1140, "Unknown"},
	{1141, "Unknown"},
	{1142, "Unknown"},
	{1143, "Unknown"},
	{1144, "Mega Flight Dragon"},
	{1145, "Unknown"},
	{1146, "Unknown"},
	{1147, "Unknown"},
	{1148, "Unknown"},
	{1149, "Unknown"},
	{1150, "Unknown"},
	{1151, "Unknown"},
	{1152, "Unknown"},
	{1153, "Unknown"},
	{1154, "Unknown"},
	{1155, "Flying White Dragon"},
	{1156, "Golden Fire Dragon"},
	{1157, "Ultra Destruction Dragon"},
	{1158, "Arcade Machine"},
	{1159, "8-Bit Shooter"},
	{1160, "The Pixelator"},
	{1161, "G-6155 Spy Hunter"},
	{1162, "Interdiver"},
	{1163, "Aerial Spyhunter"},
	{1164, "Slime Shooter"},
	{1165, "Slime Exploder"},
	{1166, "Slime Streamer"},
	{1167, "Terror Dog"},
	{1168, "Terror Dog Destroyer"},
	{1169, "Soaring Terror Dog"},
	{1170, "Ancient Psychic Tandem War Elephant"},
	{1171, "Cosmic Squid"},
	{1172, "Psychic Submarine"},
	{1173, "BMO"},
	{1174, "DOGMO"},
	{1175, "SNAKEMO"},
	{1176, "Jakemobile"},
	{1177, "Snail Dude Jake"},
	{1178, "Hover Jake"},
	{1179, "Lumpy Car"},
	{1181, "Lumpy Land Whale"},
	{1180, "Lumpy Truck"},
	{1182, "Lunatic Amp"},
	{1183, "Shadow Scorpion"},
	{1184, "Heavy Metal Monster"},
	{1185, "B.A.'s Van"},
	{1186, "Fool Smasher"},
	{1187, "Pain Plane"},
	{1188, "Phone Home"},
	{1189, "Mobile Uplink"},
	{1190, "Super-Charged Satellite"},
	{1191, "Niffler"},
	{1192, "Sinister Scorpion"},
	{1193, "Vicious Vulture"},
	{1194, "Swooping Evil"},
	{1195, "Brutal Bloom"},
	{1196, "Crawling Creeper"},
	{1197, "Ecto-1 (2016)"},
	{1198, "Ectozer"},
	{1199, "PerfEcto"},
	{1200, "Flash 'n' Finish"},
	{1201, "Rampage Record Player"},
	{1202, "Stripe's Throne"},
	{1203, "R.C. Racer"},
	{1204, "Gadget-O-Matic"},
	{1205, "Scarlet Scorpion"},
	{1206, "Hogwarts Express"},
	{1208, "Steam Warrior"},
	{1207, "Soaring Steam Plane"},
	{1209, "Enchanted Car"},
	{1210, "Shark Sub"},
	{1211, "Monstrous Mouth"},
	{1212, "IMF Scrambler"},
	{1213, "Shock Cycle"},
	{1214, "IMF Covert Jet"},
	{1215, "IMF Sports Car"},
	{1216, "IMF Tank"},
	{1217, "IMF Splorer"},
	{1218, "Sonic Speedster"},
	{1219, "Blue Typhoon"},
	{1220, "Moto Bug"},
	{1221, "The Tornado"},
	{1222, "Crabmeat"},
	{1223, "Eggcatcher"},
	{1224, "K.I.T.T."},
	{1225, "Goliath Armored Semi"},
	{1226, "K.I.T.T. Jet"},
	{1227, "Police Helicopter"},
	{1228, "Police Hovercraft"},
	{1229, "Police Plane"},
	{1230, "Bionic Steed"},
	{1231, "Bat-Raptor"},
	{1232, "Ultrabat"},
	{1233, "Batwing"},
	{1234, "The Black Thunder"},
	{1235, "Bat-Tank"},
	{1236, "Skeleton Organ"},
	{1237, "Skeleton Jukebox"},
	{1238, "Skele-Turkey"},
	{1239, "One-Eyed Willy's Pirate Ship"},
	{1240, "Fanged Fortune"},
	{1241, "Inferno Cannon"},
	{1242, "Buckbeak"},
	{1243, "Giant Owl"},
	{1244, "Fierce Falcon"},
	{1245, "Saturn's Sandworm"},
	{1247, "Haunted Vacuum"},
	{1246, "Spooky Spider"},
	{1248, "PPG Smartphone"},
	{1249, "PPG Hotline"},
	{1250, "Powerpuff Mag-Net"},
	{1253, "Mega Blast Bot"},
	{1251, "Ka-Pow Cannon"},
	{1252, "Slammin' Guitar"},
	{1254, "Octi"},
	{1255, "Super Skunk"},
	{1256, "Sonic Squid"},
	{1257, "T-Car"},
	{1258, "T-Forklift"},
	{1259, "T-Plane"},
	{1260, "Spellbook of Azarath"},
	{1261, "Raven Wings"},
	{1262, "Giant Hand"},
	{1263, "Titan Robot"},
	{1264, "T-Rocket"},
	{1265, "Robot Retriever"}};

minifig_creator_dialog::minifig_creator_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Figure Creator"));
	setObjectName("figure_creator");
	setMinimumSize(QSize(500, 150));

	QVBoxLayout* vbox_panel = new QVBoxLayout();

	QComboBox* combo_figlist = new QComboBox();
	QStringList filterlist;

	for (const auto& [figure, figure_name] : list_minifigs)
	{
		QString name = QString::fromStdString(figure_name);
		combo_figlist->addItem(name, QVariant(figure));
		filterlist << std::move(name);
	}

	combo_figlist->addItem(tr("--Unknown--"), QVariant(0xFFFF));
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

	QHBoxLayout* hbox_number = new QHBoxLayout();
	QLabel* label_number = new QLabel(tr("Figure Number:"));
	QLineEdit* edit_number = new QLineEdit(QString::fromStdString(std::to_string(0)));
	QRegularExpressionValidator* rxv = new QRegularExpressionValidator(QRegularExpression("\\d*"), this);
	edit_number->setValidator(rxv);
	hbox_number->addWidget(label_number);
	hbox_number->addWidget(edit_number);
	vbox_panel->addLayout(hbox_number);

	QHBoxLayout* hbox_buttons = new QHBoxLayout();
	QPushButton* btn_create = new QPushButton(tr("Create"), this);
	QPushButton* btn_cancel = new QPushButton(tr("Cancel"), this);
	hbox_buttons->addStretch();
	hbox_buttons->addWidget(btn_create);
	hbox_buttons->addWidget(btn_cancel);
	vbox_panel->addLayout(hbox_buttons);

	setLayout(vbox_panel);

	connect(combo_figlist, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index)
		{
			const u16 fig_info = combo_figlist->itemData(index).toUInt();
			if (fig_info != 0xFFFF)
			{
				edit_number->setText(QString::number(fig_info));
			}
		});

	connect(btn_create, &QAbstractButton::clicked, this, [=, this]()
		{
			bool ok_num = false;
			const u16 fig_num = edit_number->text().toUInt(&ok_num) & 0xFFFF;
			if (!ok_num)
			{
				QMessageBox::warning(this, tr("Error converting value"), tr("Figure number entered is invalid!"), QMessageBox::Ok);
				return;
			}
			const auto found_figure = list_minifigs.find(fig_num);
			if (found_figure != list_minifigs.cend())
			{
				s_last_figure_path += QString::fromStdString(found_figure->second + ".bin");
			}
			else
			{
				s_last_figure_path += QString("Unknown(%1).bin").arg(fig_num);
			}

			m_file_path = QFileDialog::getSaveFileName(this, tr("Create Figure File"), s_last_figure_path, tr("Dimensions Figure (*.bin);;"));
			if (m_file_path.isEmpty())
			{
				return;
			}

			fs::file dim_file(m_file_path.toStdString(), fs::read + fs::write + fs::create);
			if (!dim_file)
			{
				QMessageBox::warning(this, tr("Failed to create minifig file!"), tr("Failed to create minifig file:\n%1").arg(m_file_path), QMessageBox::Ok);
				return;
			}

			std::array<u8, 0x2D * 0x04> file_data{};
			g_dimensionstoypad.create_blank_character(file_data, fig_num);

			dim_file.write(file_data.data(), file_data.size());
			dim_file.close();

			s_last_figure_path = QFileInfo(m_file_path).absolutePath() + "/";
			accept();
		});

	connect(btn_cancel, &QAbstractButton::clicked, this, &QDialog::reject);

	connect(co_compl, QOverload<const QString&>::of(&QCompleter::activated), [=](const QString& text)
		{
			combo_figlist->setCurrentIndex(combo_figlist->findText(text));
		});
}

QString minifig_creator_dialog::get_file_path() const
{
	return m_file_path;
}

minifig_move_dialog::minifig_move_dialog(QWidget* parent, u8 old_index)
	: QDialog(parent)
{
	setWindowTitle(tr("Figure Mover"));
	setObjectName("figure_mover");
	setMinimumSize(QSize(500, 150));

	auto* grid_panel = new QGridLayout();

	add_minifig_position(grid_panel, 0, 0, 0, old_index);
	grid_panel->addWidget(new QLabel(tr("")), 0, 1);
	add_minifig_position(grid_panel, 1, 0, 2, old_index);
	grid_panel->addWidget(new QLabel(tr(""), this), 0, 3);
	add_minifig_position(grid_panel, 2, 0, 4, old_index);

	add_minifig_position(grid_panel, 3, 1, 0, old_index);
	add_minifig_position(grid_panel, 4, 1, 1, old_index);
	grid_panel->addWidget(new QLabel(tr("")), 1, 2);
	add_minifig_position(grid_panel, 5, 1, 3, old_index);
	add_minifig_position(grid_panel, 6, 1, 4, old_index);

	setLayout(grid_panel);
}

void minifig_move_dialog::add_minifig_position(QGridLayout* grid_panel, u8 index, u8 row, u8 column, u8 old_index)
{
	ensure(index < figure_slots.size());

	auto* vbox_panel = new QVBoxLayout();

	if (figure_slots[index])
	{
		const auto found_figure = list_minifigs.find(figure_slots[index].value());
		if (found_figure != list_minifigs.cend())
		{
			vbox_panel->addWidget(new QLabel(tr(found_figure->second.c_str())));
		}
	}
	else
	{
		vbox_panel->addWidget(new QLabel(tr("None")));
	}
	auto* btn_move = new QPushButton(tr("Move Here"), this);
	if (old_index == index)
	{
		btn_move->setText(tr("Pick up and Place"));
	}
	vbox_panel->addWidget(btn_move);
	connect(btn_move, &QAbstractButton::clicked, this, [this, index]
		{
			m_index = index;
			m_pad = index == 1                             ? 1 :
		            index == 0 || index == 3 || index == 4 ? 2 :
		                                                     3;
			accept();
		});

	grid_panel->addLayout(vbox_panel, row, column);
}

u8 minifig_move_dialog::get_new_pad() const
{
	return m_pad;
}

u8 minifig_move_dialog::get_new_index() const
{
	return m_index;
}

dimensions_dialog::dimensions_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Dimensions Manager"));
	setObjectName("dimensions_manager");
	setAttribute(Qt::WA_DeleteOnClose);
	setMinimumSize(QSize(800, 200));

	QVBoxLayout* vbox_panel = new QVBoxLayout();
	QVBoxLayout* vbox_group = new QVBoxLayout();
	QHBoxLayout* hbox_group_1 = new QHBoxLayout();
	QHBoxLayout* hbox_group_2 = new QHBoxLayout();

	QGroupBox* group_figures = new QGroupBox(tr("Active Dimensions Figures:"));

	add_minifig_slot(hbox_group_1, 2, 0);
	hbox_group_1->addSpacerItem(new QSpacerItem(50, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
	add_minifig_slot(hbox_group_1, 1, 1);
	hbox_group_1->addSpacerItem(new QSpacerItem(50, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
	add_minifig_slot(hbox_group_1, 3, 2);

	add_minifig_slot(hbox_group_2, 2, 3);
	add_minifig_slot(hbox_group_2, 2, 4);
	hbox_group_2->addSpacerItem(new QSpacerItem(50, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
	add_minifig_slot(hbox_group_2, 3, 5);
	add_minifig_slot(hbox_group_2, 3, 6);

	vbox_group->addLayout(hbox_group_1);
	vbox_group->addSpacerItem(new QSpacerItem(0, 20, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
	vbox_group->addLayout(hbox_group_2);
	group_figures->setLayout(vbox_group);
	vbox_panel->addWidget(group_figures);
	setLayout(vbox_panel);
}

dimensions_dialog::~dimensions_dialog()
{
	inst = nullptr;
}

dimensions_dialog* dimensions_dialog::get_dlg(QWidget* parent)
{
	if (inst == nullptr)
		inst = new dimensions_dialog(parent);

	return inst;
}

void dimensions_dialog::add_minifig_slot(QHBoxLayout* layout, u8 pad, u8 index)
{
	ensure(index < figure_slots.size());

	QVBoxLayout* vbox_layout = new QVBoxLayout();

	QHBoxLayout* hbox_name_move = new QHBoxLayout();
	QHBoxLayout* hbox_actions = new QHBoxLayout();

	QPushButton* clear_btn = new QPushButton(tr("Clear"));
	QPushButton* create_btn = new QPushButton(tr("Create"));
	QPushButton* load_btn = new QPushButton(tr("Load"));
	QPushButton* move_btn = new QPushButton(tr("Move"));

	m_edit_figures[index] = new QLineEdit();
	m_edit_figures[index]->setEnabled(false);
	if (figure_slots[index])
	{
		const auto found_figure = list_minifigs.find(figure_slots[index].value());
		if (found_figure != list_minifigs.cend())
		{
			m_edit_figures[index]->setText(QString::fromStdString(found_figure->second));
		}
		else
		{
			m_edit_figures[index]->setText(tr("Unknown Figure"));
		}
	}
	else
	{
		m_edit_figures[index]->setText(tr("None"));
	}

	connect(clear_btn, &QAbstractButton::clicked, this, [this, pad, index]
		{
			clear_figure(pad, index);
		});
	connect(create_btn, &QAbstractButton::clicked, this, [this, pad, index]
		{
			create_figure(pad, index);
		});
	connect(load_btn, &QAbstractButton::clicked, this, [this, pad, index]
		{
			load_figure(pad, index);
		});
	connect(move_btn, &QAbstractButton::clicked, this, [this, pad, index]
		{
			if (figure_slots[index])
			{
				move_figure(pad, index);
			}
		});

	hbox_name_move->addWidget(m_edit_figures[index]);
	hbox_name_move->addWidget(move_btn);
	hbox_actions->addWidget(clear_btn);
	hbox_actions->addWidget(create_btn);
	hbox_actions->addWidget(load_btn);

	vbox_layout->addLayout(hbox_name_move);
	vbox_layout->addLayout(hbox_actions);

	layout->addLayout(vbox_layout);
}

void dimensions_dialog::clear_figure(u8 pad, u8 index)
{
	ensure(index < figure_slots.size());

	if (figure_slots[index])
	{
		g_dimensionstoypad.remove_figure(pad, index, true, true);
		figure_slots[index] = std::nullopt;
		m_edit_figures[index]->setText(tr("None"));
	}
}

void dimensions_dialog::create_figure(u8 pad, u8 index)
{
	ensure(index < figure_slots.size());
	minifig_creator_dialog create_dlg(this);
	if (create_dlg.exec() == Accepted)
	{
		load_figure_path(pad, index, create_dlg.get_file_path());
	}
}

void dimensions_dialog::load_figure(u8 pad, u8 index)
{
	ensure(index < figure_slots.size());
	const QString file_path = QFileDialog::getOpenFileName(this, tr("Select Dimensions File"), s_last_figure_path, tr("Dimensions Figure (*.bin);;"));
	if (file_path.isEmpty())
	{
		return;
	}

	s_last_figure_path = QFileInfo(file_path).absolutePath() + "/";

	load_figure_path(pad, index, file_path);
}

void dimensions_dialog::move_figure(u8 pad, u8 index)
{
	ensure(index < figure_slots.size());
	minifig_move_dialog move_dlg(this, index);
	g_dimensionstoypad.temp_remove(index);
	if (move_dlg.exec() == Accepted)
	{
		g_dimensionstoypad.move_figure(move_dlg.get_new_pad(), move_dlg.get_new_index(), pad, index);
		if (index != move_dlg.get_new_index())
		{
			figure_slots[move_dlg.get_new_index()] = figure_slots[index];
			m_edit_figures[move_dlg.get_new_index()]->setText(m_edit_figures[index]->text());
			figure_slots[index] = std::nullopt;
			m_edit_figures[index]->setText(tr("None"));
		}
	}
	else
	{
		g_dimensionstoypad.cancel_remove(index);
	}
}

void dimensions_dialog::load_figure_path(u8 pad, u8 index, const QString& path)
{
	fs::file dim_file(path.toStdString(), fs::read + fs::write + fs::lock);
	if (!dim_file)
	{
		QMessageBox::warning(this, tr("Failed to open the figure file!"), tr("Failed to open the figure file(%1)!\nFile may already be in use on the base.").arg(path), QMessageBox::Ok);
		return;
	}

	std::array<u8, 0x2D * 0x04> data;
	if (dim_file.read(data.data(), data.size()) != data.size())
	{
		QMessageBox::warning(this, tr("Failed to read the figure file!"), tr("Failed to read the figure file(%1)!\nFile was too small.").arg(path), QMessageBox::Ok);
		return;
	}

	clear_figure(pad, index);

	const u32 fig_num = g_dimensionstoypad.load_figure(data, std::move(dim_file), pad, index, true);

	figure_slots[index] = fig_num;
	const auto name = list_minifigs.find(fig_num);
	if (name != list_minifigs.cend())
	{
		m_edit_figures[index]->setText(QString::fromStdString(name->second));
	}
	else
	{
		const auto blank_name = list_tokens.find(fig_num);
		if (blank_name != list_tokens.cend())
		{
			m_edit_figures[index]->setText(QString::fromStdString(blank_name->second));
		}
		else
		{
			m_edit_figures[index]->setText("Blank Tag");
		}
	}
}
