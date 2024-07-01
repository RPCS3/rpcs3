#include "stdafx.h"
#include "Utilities/File.h"
#include "dimensions_dialog.h"
#include "Emu/Io/Dimensions.h"

#include "util/asm.hpp"

#include <locale>

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
std::array<std::optional<u16>, 7> figure_slots = {};
static QString s_last_figure_path;

LOG_CHANNEL(dimensions_log, "dimensions");

const std::map<const u16, const std::string> list_minifigs = {
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
	{770, "Unknown 770"},
	{1000, "PoliceCar"},
	{1001, "*AerialSquadCar"},
	{1002, "*MissileStriker"},
	{1003, "GravitySprinter"},
	{1004, "*StreetShredder"},
	{1005, "*SkyClobberer"},
	{1006, "Batmobile"},
	{1007, "*Batblaster"},
	{1008, "*SonicBatray"},
	{1009, "Benny'sSpaceship"},
	{1010, "*Lasercraft"},
	{1011, "*TheAnnihilator"},
	{1012, "DeLoreanTimeMachine"},
	{1013, "*ElectricTimeMachine"},
	{1014, "*UltraTimeMachine"},
	{1015, "Hoverboard"},
	{1016, "*CycloneBoard"},
	{1017, "*UltimateHoverjet"},
	{1018, "EagleInterceptor"},
	{1019, "*EagleSkyBlazer"},
	{1020, "*EagleSwoopDiver"},
	{1021, "SwampSkimmer"},
	{1022, "*Cragger'sFireship"},
	{1023, "*CrocCommandSub"},
	{1024, "Cyber-Guard"},
	{1025, "*Cyber-Wrecker"},
	{1026, "*LaserRobotWalker"},
	{1027, "K-9"},
	{1028, "*K-9RuffRover"},
	{1029, "*K-9LaserCutter"},
	{1030, "TARDIS"},
	{1031, "*Laser-PulseTARDIS"},
	{1032, "*Energy-BurstTARDIS"},
	{1033, "Emmet'sExcavator"},
	{1034, "*DestroyDozer"},
	{1035, "*Destruct-o-Mech"},
	{1036, "WingedMonkey"},
	{1037, "*BattleMonkey"},
	{1038, "*CommanderMonkey"},
	{1039, "AxeChariot"},
	{1040, "*AxeHurler"},
	{1041, "*SoaringChariot"},
	{1042, "ShelobtheGreat"},
	{1043, "*8-LeggedStalker"},
	{1044, "*PoisonSlinger"},
	{1045, "Homer'sCar"},
	{1047, "*TheSubmaHomer"},
	{1046, "*TheHomercraft"},
	{1048, "Taunt-o-Vision"},
	{1050, "*TheMechaHomer"},
	{1049, "*BlastCam"},
	{1051, "Velociraptor"},
	{1053, "*VenomRaptor"},
	{1052, "*SpikeAttackRaptor"},
	{1054, "Gyrosphere"},
	{1055, "*SonicBeamGyrosphere"},
	{1056, "*SpeedBoostGyrosphere"},
	{1057, "ClownBike"},
	{1058, "*CannonBike"},
	{1059, "*Anti-GravityRocketBike"},
	{1060, "MightyLionRider"},
	{1061, "*LionBlazer"},
	{1062, "*FireLion"},
	{1063, "ArrowLauncher"},
	{1064, "*SeekingShooter"},
	{1065, "*TripleBallista"},
	{1066, "MysteryMachine"},
	{1067, "*MysteryTow&Go"},
	{1068, "*MysteryMonster"},
	{1069, "BoulderBomber"},
	{1070, "*BoulderBlaster"},
	{1071, "*CycloneJet"},
	{1072, "StormFighter"},
	{1073, "*LightningJet"},
	{1074, "*Electro-Shooter"},
	{1075, "BladeBike"},
	{1076, "*FlightFireBike"},
	{1077, "*BladesofFire"},
	{1078, "SamuraiMech"},
	{1079, "*SamuraiShooter"},
	{1080, "*SoaringSamuraiMech"},
	{1081, "CompanionCube"},
	{1082, "*LaserDeflector"},
	{1083, "*GoldHeartEmitter"},
	{1084, "SentryTurret"},
	{1085, "*TurretStriker"},
	{1086, "*FlightTurretCarrier"},
	{1087, "ScoobySnack"},
	{1088, "*ScoobyFireSnack"},
	{1089, "*ScoobyGhostSnack"},
	{1090, "CloudCuckooCar"},
	{1091, "*X-StreamSoaker"},
	{1092, "*RainbowCannon"},
	{1093, "InvisibleJet"},
	{1094, "*LaserShooter"},
	{1095, "*TorpedoBomber"},
	{1096, "NinjaCopter"},
	{1097, "*Glaciator"},
	{1098, "*FreezeFighter"},
	{1099, "TravellingTimeTrain"},
	{1100, "*FlightTimeTrain"},
	{1101, "*MissileBlastTimeTrain"},
	{1102, "AquaWatercraft"},
	{1103, "*SevenSeasSpeeder"},
	{1104, "*TridentofFire"},
	{1105, "DrillDriver"},
	{1106, "*BaneDig'n'Drill"},
	{1107, "*BaneDrill'n'Blast"},
	{1108, "QuinnMobile"},
	{1109, "*QuinnUltraRacer"},
	{1110, "*MissileLauncher"},
	{1111, "TheJoker'sChopper"},
	{1112, "*MischievousMissileBlaster"},
	{1113, "*Lock'n'LaserJet"},
	{1114, "HoverPod"},
	{1115, "*KryptonStriker"},
	{1116, "*SuperStealthPod"},
	{1117, "Dalek"},
	{1118, "*Fire'n'RideDalek"},
	{1119, "*SilverShooterDalek"},
	{1120, "Ecto-1"},
	{1121, "*Ecto-1Blaster"},
	{1122, "*Ecto-1WaterDiver"},
	{1123, "GhostTrap"},
	{1124, "*GhostStun'n'Trap"},
	{1125, "*ProtonZapper"},
	{1126, "Unknown"},
	{1127, "Unknown"},
	{1128, "Unknown"},
	{1129, "Unknown"},
	{1130, "Unknown"},
	{1131, "Unknown"},
	{1132, "Lloyd'sGoldenDragon"},
	{1133, "*SwordProjectorDragon"},
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
	{1144, "*MegaFlightDragon"},
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
	{1155, "FlyingWhiteDragon"},
	{1156, "*GoldenFireDragon"},
	{1157, "*UltraDestructionDragon"},
	{1158, "ArcadeMachine"},
	{1159, "*8-BitShooter"},
	{1160, "*ThePixelator"},
	{1161, "G-6155SpyHunter"},
	{1162, "*Interdiver"},
	{1163, "*AerialSpyhunter"},
	{1164, "SlimeShooter"},
	{1165, "*SlimeExploder"},
	{1166, "*SlimeStreamer"},
	{1167, "TerrorDog"},
	{1168, "*TerrorDogDestroyer"},
	{1169, "*SoaringTerrorDog"},
	{1170, "AncientPsychicTandemWarElephant"},
	{1171, "*CosmicSquid"},
	{1172, "*PsychicSubmarine"},
	{1173, "BMO"},
	{1174, "*DOGMO"},
	{1175, "*SNAKEMO"},
	{1176, "Jakemobile"},
	{1177, "*SnailDudeJake"},
	{1178, "*HoverJake"},
	{1179, "LumpyCar"},
	{1181, "*LumpyLandWhale"},
	{1180, "*LumpyTruck"},
	{1182, "LunaticAmp"},
	{1183, "*ShadowScorpion"},
	{1184, "*HeavyMetalMonster"},
	{1185, "B.A.'sVan"},
	{1186, "*FoolSmasher"},
	{1187, "*PainPlane"},
	{1188, "PhoneHome"},
	{1189, "*MobileUplink"},
	{1190, "*Super-ChargedSatellite"},
	{1191, "Niffler"},
	{1192, "*SinisterScorpion"},
	{1193, "*ViciousVulture"},
	{1194, "SwoopingEvil"},
	{1195, "*BrutalBloom"},
	{1196, "*CrawlingCreeper"},
	{1197, "Ecto-1(2016)"},
	{1198, "*Ectozer"},
	{1199, "*PerfEcto"},
	{1200, "Flash'n'Finish"},
	{1201, "*RampageRecordPlayer"},
	{1202, "*Stripe'sThrone"},
	{1203, "R.C.Racer"},
	{1204, "*Gadget-O-Matic"},
	{1205, "*ScarletScorpion"},
	{1206, "HogwartsExpress"},
	{1208, "*SteamWarrior"},
	{1207, "*SoaringSteamPlane"},
	{1209, "EnchantedCar"},
	{1210, "*SharkSub"},
	{1211, "*MonstrousMouth"},
	{1212, "IMFScrambler"},
	{1213, "*ShockCycle"},
	{1214, "*IMFCovertJet"},
	{1215, "IMFSportsCar"},
	{1216, "*IMFTank"},
	{1217, "*IMFSplorer"},
	{1218, "SonicSpeedster"},
	{1219, "*BlueTyphoon"},
	{1220, "*MotoBug"},
	{1221, "TheTornado"},
	{1222, "*Crabmeat"},
	{1223, "*Eggcatcher"},
	{1224, "K.I.T.T."},
	{1225, "*GoliathArmoredSemi"},
	{1226, "*K.I.T.T.Jet"},
	{1227, "PoliceHelicopter"},
	{1228, "*PoliceHovercraft"},
	{1229, "*PolicePlane"},
	{1230, "BionicSteed"},
	{1231, "*Bat-Raptor"},
	{1232, "*Ultrabat"},
	{1233, "Batwing"},
	{1234, "*TheBlackThunder"},
	{1235, "*Bat-Tank"},
	{1236, "SkeletonOrgan"},
	{1237, "*SkeletonJukebox"},
	{1238, "*Skele-Turkey"},
	{1239, "One-EyedWilly'sPirateShip"},
	{1240, "*FangedFortune"},
	{1241, "*InfernoCannon"},
	{1242, "Buckbeak"},
	{1243, "*GiantOwl"},
	{1244, "*FierceFalcon"},
	{1245, "Saturn'sSandworm"},
	{1247, "*HauntedVacuum"},
	{1246, "*SpookySpider"},
	{1248, "PPGSmartphone"},
	{1249, "*PPGHotline"},
	{1250, "*PowerpuffMag-Net"},
	{1253, "MegaBlastBot"},
	{1251, "*Ka-PowCannon"},
	{1252, "*Slammin'Guitar"},
	{1254, "Octi"},
	{1255, "*SuperSkunk"},
	{1256, "*SonicSquid"},
	{1257, "T-Car"},
	{1258, "*T-Forklift"},
	{1259, "*T-Plane"},
	{1260, "SpellbookofAzarath"},
	{1261, "*RavenWings"},
	{1262, "*GiantHand"},
	{1263, "TitanRobot"},
	{1264, "*T-Rocket"},
	{1265, "*RobotRetriever"}};

minifig_creator_dialog::minifig_creator_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Figure Creator"));
	setObjectName("figure_creator");
	setMinimumSize(QSize(500, 150));

	QVBoxLayout* vbox_panel = new QVBoxLayout();

	QComboBox* combo_figlist = new QComboBox();
	QStringList filterlist;
	u32 first_entry = 0;

	for (const auto& entry : list_minifigs)
	{
		const auto figure = entry.first;
		{
			combo_figlist->addItem(QString::fromStdString(entry.second), QVariant(figure));
			filterlist << entry.second.c_str();
			if (first_entry == 0)
			{
				first_entry = figure;
			}
		}
	}

	combo_figlist->addItem(tr("--Unknown--"), QVariant(0xFFFF));
	combo_figlist->setEditable(true);
	combo_figlist->setInsertPolicy(QComboBox::NoInsert);

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
	QLineEdit* edit_number = new QLineEdit(QString::fromStdString(std::to_string(first_entry)));
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
			if (found_figure != list_minifigs.end())
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

			file_data[0] = 0x04;
			file_data[6] = 0x80;

			for (u8 i = 1; i < 6; i++)
			{
				file_data[i] = rand() % 255;
			}
			write_to_ptr<be_t<u16>>(file_data.data(), 0x0E, fig_num);

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
	auto* vbox_panel = new QVBoxLayout();

	if (figure_slots[index])
	{
		const auto found_figure = list_minifigs.find(figure_slots[index].value());
		if (found_figure != list_minifigs.end())
		{
			vbox_panel->addWidget(new QLabel(tr(found_figure->second.c_str())));
		}
	}
	else
	{
		vbox_panel->addWidget(new QLabel(tr("None")));
	}
	if (old_index != index)
	{
		auto* btn_move = new QPushButton(tr("Move Here"), this);
		vbox_panel->addWidget(btn_move);
		connect(btn_move, &QAbstractButton::clicked, this, [this, index]
			{
				m_index = index;
				m_pad = index == 2                               ? 1 :
			            (index == 1 || index == 4 || index == 5) ? 2 :
			                                                       3;
				accept();
			});
	}

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
	setMinimumSize(QSize(1200, 500));

	QVBoxLayout* vbox_panel = new QVBoxLayout();

	QGroupBox* group_figures = new QGroupBox(tr("Active Dimensions Figures:"));
	QGridLayout* grid_group = new QGridLayout();

	add_minifig_slot(grid_group, 2, 1, 0, 0);
	grid_group->addWidget(new QLabel(tr("")), 0, 1);
	add_minifig_slot(grid_group, 1, 2, 0, 2);
	grid_group->addWidget(new QLabel(tr("")), 0, 1);
	add_minifig_slot(grid_group, 3, 3, 0, 4);

	add_minifig_slot(grid_group, 2, 4, 1, 0);
	add_minifig_slot(grid_group, 2, 5, 1, 1);
	grid_group->addWidget(new QLabel(tr("")), 0, 1);
	add_minifig_slot(grid_group, 3, 6, 1, 3);
	add_minifig_slot(grid_group, 3, 7, 1, 4);

	group_figures->setLayout(grid_group);
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

void dimensions_dialog::add_minifig_slot(QGridLayout* grid_group, u8 pad, u8 index, u8 row, u8 column)
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
		if (found_figure != list_minifigs.end())
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
			if (figure_slots[index] && figure_slots[index] != 0)
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

	grid_group->addLayout(vbox_layout, row, column);
}

void dimensions_dialog::clear_figure(u8 pad, u8 index)
{
	ensure(index < figure_slots.size());

	if (figure_slots[index])
	{
		g_dimensionstoypad.remove_figure(pad, index, true);
		figure_slots[index] = 0;
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
	if (move_dlg.exec() == Accepted)
	{
		g_dimensionstoypad.move_figure(move_dlg.get_new_pad(), move_dlg.get_new_index(), pad, index);
		figure_slots[move_dlg.get_new_index() - 1] = figure_slots[index];
		m_edit_figures[move_dlg.get_new_index() - 1]->setText(m_edit_figures[index]->text());
		figure_slots[index] = 0;
		m_edit_figures[index]->setText(tr("None"));
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

	u16 fig_num = g_dimensionstoypad.load_figure(data, std::move(dim_file), pad, index);

	figure_slots[index] = fig_num;
	auto name = list_minifigs.find(fig_num);
	if (name != list_minifigs.end())
	{
		m_edit_figures[index]->setText(QString::fromStdString(name->second));
	}
	else
	{
		m_edit_figures[index]->setText(tr("Unknown Figure"));
	}
}
