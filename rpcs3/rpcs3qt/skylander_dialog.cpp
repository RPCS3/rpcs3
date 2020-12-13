#include <mutex>
#include "Utilities/File.h"
#include "Crypto/md5.h"
#include "Crypto/aes.h"
#include "skylander_dialog.h"
#include "Utilities/BEType.h"
#include "Emu/Io/Skylander.h"

#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QVBoxLayout>

skylander_dialog* skylander_dialog::inst = nullptr;

const std::map<const std::pair<const u16, const u16>, const std::string> list_skylanders = {
    {{0, 0}, "Whirlwind"},
    {{1, 0}, "Sonic Boom"},
    {{2, 0}, "Warnado"},
    {{3, 0}, "Lightning Rod"},
    {{4, 0}, "Bash"},
    {{5, 0}, "Terrafin"},
    {{6, 0}, "Dino-Rang"},
    {{7, 0}, "Prism Break"},
    {{8, 0}, "Sunburn"},
    {{9, 0}, "Eruptor"},
    {{10, 0}, "Ignitor"},
    {{11, 0}, "Flameslinger"},
    {{12, 0}, "Zap"},
    {{13, 0}, "Wham-Shell"},
    {{14, 0}, "Gill Grunt"},
    {{15, 0}, "Slam Bam"},
    {{16, 0}, "Spyro"},
    {{17, 0}, "Voodood"},
    {{18, 0}, "Double Trouble"},
    {{19, 0}, "Trigger Happy"},
    {{20, 0}, "Drobot"},
    {{21, 0}, "Drill Sergeant"},
    {{22, 0}, "Boomer"},
    {{23, 0}, "Wrecking Ball"},
    {{24, 0}, "Camo"},
    {{25, 0}, "Zook"},
    {{26, 0}, "Stealth Elf"},
    {{27, 0}, "Stump Smash"},
    {{28, 0}, "Dark Spyro"},
    {{29, 0}, "Hex"},
    {{30, 0}, "Chop Chop"},
    {{31, 0}, "Ghost Roaster"},
    {{32, 0}, "Cynder"},
    {{100, 0}, "Jet Vac"},
    {{101, 0}, "Swarm"},
    {{102, 0}, "Crusher"},
    {{103, 0}, "Flashwing"},
    {{104, 0}, "Hot Head"},
    {{105, 0}, "Hot Dog"},
    {{106, 0}, "Chill"},
    {{107, 0}, "Thumpback"},
    {{108, 0}, "Pop Fizz"},
    {{109, 0}, "Ninjini"},
    {{110, 0}, "Bouncer"},
    {{111, 0}, "Sprocket"},
    {{112, 0}, "Tree Rex"},
    {{113, 0}, "Shroomboom"},
    {{114, 0}, "Eye-Brawl"},
    {{115, 0}, "Fright Rider"},
    {{200, 0}, "Anvil Rain"},
    {{201, 0}, "Treasure Chest"},
    {{202, 0}, "Healing Elixer"},
    {{203, 0}, "Ghost Swords"},
    {{204, 0}, "Time Twister"},
    {{205, 0}, "Sky-Iron Shield"},
    {{206, 0}, "Winged Boots"},
    {{207, 0}, "Sparx Dragonfly"},
    {{208, 0}, "Dragonfire Cannon"},
    {{209, 0}, "Scorpion Striker Catapult"},
    {{210, 0}, "Magic Traps"},
    {{211, 0}, "Water Traps"},
    {{212, 0}, "Air Traps"},
    {{213, 0}, "Undead Traps"},
    {{214, 0}, "Tech Traps"},
    {{215, 0}, "Fire Traps"},
    {{216, 0}, "Earth Traps"},
    {{217, 0}, "Life Traps"},
    {{218, 0}, "Dark Traps"},
    {{219, 0}, "Light Traps"},
    {{220, 0}, "Kaos Traps"},
    {{230, 0}, "Hand Of Fate"},
    {{231, 0}, "Piggy Bank"},
    {{232, 0}, "Rocket Ram"},
    {{233, 0}, "Tiki Speaky"},
    {{300, 0}, "Dragons Peak"},
    {{301, 0}, "Empire of Ice"},
    {{302, 0}, "Pirate Seas"},
    {{303, 0}, "Darklight Crypt"},
    {{304, 0}, "Volcanic Vault"},
    {{305, 0}, "Mirror Of Mystery"},
    {{306, 0}, "Nightmare Express"},
    {{307, 0}, "Sunscraper Spire"},
    {{308, 0}, "Midnight Museum"},
    {{404, 0}, "Legendary Bash"},
    {{416, 0}, "Legendary Spyro"},
    {{419, 0}, "Legendary Trigger Happy"},
    {{430, 0}, "Legendary Chop Chop"},
    {{450, 0}, "Gusto"},
    {{451, 0}, "Thunderbolt"},
    {{452, 0}, "Fling Kong"},
    {{453, 0}, "Blades"},
    {{454, 0}, "Wallop"},
    {{455, 0}, "Head Rush"},
    {{456, 0}, "Fist Bump"},
    {{457, 0}, "Rocky Roll"},
    {{458, 0}, "Wildfire"},
    {{459, 0}, "Ka Boom"},
    {{460, 0}, "Trail Blazer"},
    {{461, 0}, "Torch"},
    {{462, 0}, "Snap Shot"},
    {{463, 0}, "Lob Star"},
    {{464, 0}, "Flip Wreck"},
    {{465, 0}, "Echo"},
    {{466, 0}, "Blastermind"},
    {{467, 0}, "Enigma"},
    {{468, 0}, "Deja Vu"},
    {{469, 0}, "Cobra Cadabra"},
    {{470, 0}, "Jawbreaker"},
    {{471, 0}, "Gearshift"},
    {{472, 0}, "Chopper"},
    {{473, 0}, "Tread Head"},
    {{474, 0}, "Bushwhack"},
    {{475, 0}, "Tuff Luck"},
    {{476, 0}, "Food Fight"},
    {{477, 0}, "High Five"},
    {{478, 0}, "Krypt King"},
    {{479, 0}, "Short Cut"},
    {{480, 0}, "Bat Spin"},
    {{481, 0}, "Funny Bone"},
    {{482, 0}, "Knight light"},
    {{483, 0}, "Spotlight"},
    {{484, 0}, "Knight Mare"},
    {{485, 0}, "Blackout"},
    {{502, 0}, "Bop"},
    {{503, 0}, "Spry"},
    {{504, 0}, "Hijinx"},
    {{505, 0}, "Terrabite"},
    {{506, 0}, "Breeze"},
    {{507, 0}, "Weeruptor"},
    {{508, 0}, "Pet Vac"},
    {{509, 0}, "Small Fry"},
    {{510, 0}, "Drobit"},
    {{514, 0}, "Gill Runt"},
    {{519, 0}, "Trigger Snappy"},
    {{526, 0}, "Whisper Elf"},
    {{540, 0}, "Barkley"},
    {{541, 0}, "Thumpling"},
    {{542, 0}, "Mini Jini"},
    {{543, 0}, "Eye Small"},
    {{601, 0}, "King Pen"},
    {{602, 0}, "Tri-Tip"},
    {{603, 0}, "Chopscotch"},
    {{604, 0}, "Boom Bloom"},
    {{605, 0}, "Pit Boss"},
    {{606, 0}, "Barbella"},
    {{607, 0}, "Air Strike"},
    {{608, 0}, "Ember"},
    {{609, 0}, "Ambush"},
    {{610, 0}, "Dr. Krankcase"},
    {{611, 0}, "Hood Sickle"},
    {{612, 0}, "Tae Kwon Crow"},
    {{613, 0}, "Golden Queen"},
    {{614, 0}, "Wolfgang"},
    {{615, 0}, "Pain-Yatta"},
    {{616, 0}, "Mysticat"},
    {{617, 0}, "Starcast"},
    {{618, 0}, "Buckshot"},
    {{619, 0}, "Aurora"},
    {{620, 0}, "Flare Wolf"},
    {{621, 0}, "Chompy Mage"},
    {{622, 0}, "Bad Juju"},
    {{623, 0}, "Grave Clobber"},
    {{624, 0}, "Blaster-Tron"},
    {{625, 0}, "Ro-Bow"},
    {{626, 0}, "Chain Reaction"},
    {{627, 0}, "Kaos"},
    {{628, 0}, "Wild Storm"},
    {{629, 0}, "Tidepool"},
    {{630, 0}, "Crash Bandicoot"},
    {{631, 0}, "Dr. Neo Cortex"},
    {{1000, 0}, "Boom Jet (Bottom)"},
    {{1001, 0}, "Free Ranger (Bottom)"},
    {{1002, 0}, "Rubble Rouser (Bottom)"},
    {{1003, 0}, "Doom Stone (Bottom)"},
    {{1004, 0}, "Blast Zone (Bottom)"},
    {{1005, 0}, "Fire Kraken (Bottom)"},
    {{1006, 0}, "Stink Bomb (Bottom)"},
    {{1007, 0}, "Grilla Drilla (Bottom)"},
    {{1008, 0}, "Hoot Loop (Bottom)"},
    {{1009, 0}, "Trap Shadow (Bottom)"},
    {{1010, 0}, "Magna Charge (Bottom)"},
    {{1011, 0}, "Spy Rise (Bottom)"},
    {{1012, 0}, "Night Shift (Bottom)"},
    {{1013, 0}, "Rattle Shake (Bottom)"},
    {{1014, 0}, "Freeze Blade (Bottom)"},
    {{1015, 0}, "Wash Buckler (Bottom)"},
    {{2000, 0}, "Boom Jet (Top)"},
    {{2001, 0}, "Free Ranger (Top)"},
    {{2002, 0}, "Rubble Rouser (Top)"},
    {{2003, 0}, "Doom Stone (Top)"},
    {{2004, 0}, "Blast Zone (Top)"},
    {{2005, 0}, "Fire Kraken (Top)"},
    {{2006, 0}, "Stink Bomb (Top)"},
    {{2007, 0}, "Grilla Drilla (Top)"},
    {{2008, 0}, "Hoot Loop (Top)"},
    {{2009, 0}, "Trap Shadow (Top)"},
    {{2010, 0}, "Magna Charge (Top)"},
    {{2011, 0}, "Spy Rise (Top)"},
    {{2012, 0}, "Night Shift (Top)"},
    {{2013, 0}, "Rattle Shake (Top)"},
    {{2014, 0}, "Freeze Blade (Top)"},
    {{2015, 0}, "Wash Buckler (Top)"},
    {{3000, 0}, "Scratch"},
    {{3001, 0}, "Pop Thorn"},
    {{3002, 0}, "Slobber Tooth"},
    {{3003, 0}, "Scorp"},
    {{3004, 0}, "Fryno"},
    {{3005, 0}, "Smolderdash"},
    {{3006, 0}, "Bumble Blast"},
    {{3007, 0}, "Zoo Lou"},
    {{3008, 0}, "Dune Bug"},
    {{3009, 0}, "Star Strike"},
    {{3010, 0}, "Countdown"},
    {{3011, 0}, "Wind Up"},
    {{3012, 0}, "Roller Brawl"},
    {{3013, 0}, "Grim Creeper"},
    {{3014, 0}, "Rip Tide"},
    {{3015, 0}, "Punk Shock"},
    {{3220, 0}, "Jet Stream"},
    {{3221, 0}, "Tomb Buggy"},
    {{3222, 0}, "Reef Ripper"},
    {{3223, 0}, "Burn Cycle"},
    {{3224, 0}, "Hot Streak"},
    {{3225, 0}, "Shark Tank"},
    {{3226, 0}, "Thump Truck"},
    {{3227, 0}, "Crypt Crusher"},
    {{3228, 0}, "Stealth Stinger"},
    {{3231, 0}, "Dive Bomber"},
    {{3232, 0}, "Sky Slicer"},
    //{{3233, 0}, "Clown Cruiser"},
    {{3234, 0}, "Gold Rusher"},
    {{3235, 0}, "Shield Striker"},
    {{3236, 0}, "Sun Runner"},
    {{3237, 0}, "Sea Shadow"},
    {{3238, 0}, "Splatter Splasher"},
    {{3239, 0}, "Soda Skimmer"},
    //{{3240, 0}, "Barrel Blaster"},
    {{3241, 0}, "Buzz Wing"},
    {{3400, 0}, "Fiesta"},
    {{3401, 0}, "High Volt"},
    {{3406, 0}, "Splat"},
    {{3411, 0}, "Smash Hit"},
    {{3412, 0}, "Spitfire"},
    {{3413, 0}, "Hurricane Jet Vac"},
    {{3414, 0}, "Double Dare Trigger Happy"},
    {{3415, 0}, "Super Shot Stealth Elf"},
    {{3416, 0}, "Shark Shooter Terrafin"},
    {{3417, 0}, "Bone Bash Roller Brawl"},
    {{3420, 0}, "Big Bubble Pop Fizz"},
    {{3421, 0}, "Lava Lance Eruptor"},
    {{3422, 0}, "Deep Dive Gill Grunt"},
    //{{3423, 0}, "Turbo Charge Donkey Kong"},
    //{{3424, 0}, "Hammer Slam Bowser"},
    {{3425, 0}, "Dive-Clops"},
    {{3426, 0}, "Astroblast"},
    {{3427, 0}, "Nightfall"},
    {{3428, 0}, "Thrillipede"},
    {{3500, 0}, "Sky Trophy"},
    {{3501, 0}, "Land Trophy"},
    {{3502, 0}, "Sea Trophy"},
    {{3503, 0}, "Kaos Trophy"},
};

QString cur_sky_file_path;

skylander_dialog::skylander_dialog(QWidget* parent)
    : QDialog(parent)
{
	setWindowTitle(tr("Skylanders Manager"));
	setObjectName("skylanders_manager");
	setAttribute(Qt::WA_DeleteOnClose);
	setMinimumSize(QSize(700, 450));

	QVBoxLayout* vbox_panel = new QVBoxLayout();

	QHBoxLayout* hbox_buttons = new QHBoxLayout();
	QPushButton* button_new   = new QPushButton(tr("New"), this);
	QPushButton* button_load  = new QPushButton(tr("Load"), this);
	hbox_buttons->addWidget(button_new);
	hbox_buttons->addWidget(button_load);
	hbox_buttons->addStretch();
	vbox_panel->addLayout(hbox_buttons);

	edit_curfile = new QLineEdit(cur_sky_file_path);
	edit_curfile->setEnabled(false);
	vbox_panel->addWidget(edit_curfile);

	QGroupBox* group_skyinfo = new QGroupBox(tr("Skylander Info"));
	QVBoxLayout* vbox_group  = new QVBoxLayout();
	combo_skylist            = new QComboBox();
	for (const auto& entry : list_skylanders)
	{
		uint qvar = (entry.first.first << 16) | entry.first.second;
		combo_skylist->addItem(QString::fromStdString(entry.second), QVariant(qvar));
	}

	combo_skylist->addItem(tr("--Unknown--"), QVariant(0xFFFFFFFF));

	QLabel* label_skyid    = new QLabel(tr("Skylander ID:"));
	edit_skyid             = new QLineEdit();
	QLabel* label_skyxp    = new QLabel(tr("Skylander XP:"));
	edit_skyxp             = new QLineEdit();
	QLabel* label_skymoney = new QLabel(tr("Skylander Money:"));
	edit_skymoney          = new QLineEdit();

	vbox_group->addWidget(combo_skylist);
	vbox_group->addWidget(label_skyid);
	vbox_group->addWidget(edit_skyid);
	vbox_group->addWidget(label_skyxp);
	vbox_group->addWidget(edit_skyxp);
	vbox_group->addWidget(label_skymoney);
	vbox_group->addWidget(edit_skymoney);

	QHBoxLayout* sub_group = new QHBoxLayout();
	sub_group->addStretch();
	button_update = new QPushButton(tr("Update"), this);
	sub_group->addWidget(button_update);
	vbox_group->addLayout(sub_group);

	vbox_group->addStretch();
	group_skyinfo->setLayout(vbox_group);

	vbox_panel->addWidget(group_skyinfo);

	setLayout(vbox_panel);

	connect(button_new, &QAbstractButton::clicked, this, &skylander_dialog::new_skylander);
	connect(button_load, &QAbstractButton::clicked, this, &skylander_dialog::load_skylander);
	connect(button_update, &QAbstractButton::clicked, this, &skylander_dialog::process_edits);

	update_edits();

	// clang-format off
	connect(combo_skylist, &QComboBox::currentTextChanged, this, [&]()
	{
		const u32 sky_info = combo_skylist->itemData(combo_skylist->currentIndex()).toUInt();
		if (sky_info != 0xFFFFFFFF)
		{
			const u16 sky_id = sky_info >> 16;
			const u16 sky_var = sky_info & 0xFFFF;
			{
				std::lock_guard lock(g_skylander.sky_mutex);
				reinterpret_cast<le_t<u32>&>(g_skylander.sky_dump[0])    = sky_info;
				reinterpret_cast<le_t<u16>&>(g_skylander.sky_dump[0x10]) = sky_id;
				reinterpret_cast<le_t<u16>&>(g_skylander.sky_dump[0x1C]) = sky_var;
				reinterpret_cast<le_t<u16>&>(g_skylander.sky_dump[0x1E]) = skylander_crc16(0xFFFF, g_skylander.sky_dump, 0x1E);
			}

			if (is_initialized())
			{
				std::lock_guard lock(g_skylander.sky_mutex);
				std::array<u8, 16> zero_array = {};
				for (u32 index = 8; index < 0x40; index++)
				{
					if ((index + 1) % 4)
					{
						set_block(index, zero_array);
					}
				}

				set_checksums();
			}

			g_skylander.sky_reload = true;
		}

		g_skylander.sky_save();
		update_edits();
	});
	// clang-format on
}

skylander_dialog::~skylander_dialog()
{
	inst = nullptr;
}

skylander_dialog* skylander_dialog::get_dlg(QWidget* parent)
{
	if (inst == nullptr)
		inst = new skylander_dialog(parent);

	return inst;
}

u16 skylander_dialog::skylander_crc16(u16 init_value, const u8* buffer, u32 size)
{
	const unsigned short CRC_CCITT_TABLE[256] = {0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 0x1231, 0x0210, 0x3273,
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

	u16 tmp;
	u16 crc = init_value;

	for (u32 i = 0; i < size; i++)
	{
		tmp = (crc >> 8) ^ buffer[i];
		crc = (crc << 8) ^ CRC_CCITT_TABLE[tmp];
	}

	return crc;
}

void skylander_dialog::get_hash(u8 block, std::array<u8, 16>& res_hash)
{
	const u8 hash_magic[0x35] = {0x20, 0x43, 0x6F, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74, 0x20, 0x28, 0x43, 0x29, 0x20, 0x32, 0x30, 0x31, 0x30, 0x20, 0x41, 0x63, 0x74, 0x69, 0x76, 0x69, 0x73, 0x69,
	    0x6F, 0x6E, 0x2E, 0x20, 0x41, 0x6C, 0x6C, 0x20, 0x52, 0x69, 0x67, 0x68, 0x74, 0x73, 0x20, 0x52, 0x65, 0x73, 0x65, 0x72, 0x76, 0x65, 0x64, 0x2E, 0x20};

	mbedtls_md5_context md5_ctx;

	mbedtls_md5_init(&md5_ctx);
	mbedtls_md5_starts_ret(&md5_ctx);
	mbedtls_md5_update_ret(&md5_ctx, g_skylander.sky_dump, 0x20);
	mbedtls_md5_update_ret(&md5_ctx, &block, 1);
	mbedtls_md5_update_ret(&md5_ctx, hash_magic, 0x35);
	mbedtls_md5_finish_ret(&md5_ctx, res_hash.data());
}

void skylander_dialog::set_block(const u8 block, const std::array<u8, 16>& to_encrypt)
{
	std::array<u8, 16> hash_result;
	get_hash(block, hash_result);

	aes_context aes_ctx;
	aes_setkey_enc(&aes_ctx, hash_result.data(), 128);

	u8 res_buf[16];
	aes_crypt_ecb(&aes_ctx, AES_ENCRYPT, to_encrypt.data(), res_buf);

	memcpy(g_skylander.sky_dump + (block * 16), res_buf, 16);
}

void skylander_dialog::get_block(const u8 block, std::array<u8, 16>& decrypted)
{
	std::array<u8, 16> hash_result;
	get_hash(block, hash_result);

	aes_context aes_ctx;
	aes_setkey_dec(&aes_ctx, hash_result.data(), 128);

	u8 res_buf[16];
	aes_crypt_ecb(&aes_ctx, AES_DECRYPT, g_skylander.sky_dump + (block * 16), res_buf);

	memcpy(decrypted.data(), res_buf, 16);
}

u8 skylander_dialog::get_active_block()
{
	std::array<u8, 16> dec_0x08;
	std::array<u8, 16> dec_0x24;
	get_block(0x08, dec_0x08);
	get_block(0x24, dec_0x24);

	return (dec_0x08[9] < dec_0x24[9]) ? 0x24 : 0x08;
}

void skylander_dialog::set_checksums()
{
	const std::array<u8, 2> sectors = {0x08, 0x24};

	for (const auto& active : sectors)
	{
		// clang-format off
		// Decrypt and hash a bunch of blocks
		auto do_crc_blocks = [&](const u16 start_crc, const std::vector<u8>& blocks) -> u16
		{
			u16 cur_crc = start_crc;
			std::array<u8, 16> decrypted_block;
			for (const auto& b : blocks)
			{
				get_block(active + b, decrypted_block);
				cur_crc = skylander_crc16(cur_crc, decrypted_block.data(), 16);
			}
			return cur_crc;
		};
		// clang-format on

		std::array<u8, 16> decrypted_header, sub_header;
		get_block(active, decrypted_header);
		get_block(active + 9, sub_header);

		// Type 4
		reinterpret_cast<le_t<u16>&>(sub_header[0x0]) = 0x0106;
		u16 res_crc                                   = skylander_crc16(0xFFFF, sub_header.data(), 16);
		reinterpret_cast<le_t<u16>&>(sub_header[0x0]) = do_crc_blocks(res_crc, {10, 12, 13});

		// Type 3
		std::array<u8, 16> zero_block{};
		res_crc = do_crc_blocks(0xFFFF, {5, 6, 8});
		for (u32 index = 0; index < 0x0E; index++)
		{
			res_crc = skylander_crc16(res_crc, zero_block.data(), 16);
		}
		reinterpret_cast<le_t<u16>&>(decrypted_header[0xA]) = res_crc;

		// Type 2
		res_crc                                             = do_crc_blocks(0xFFFF, {1, 2, 4});
		reinterpret_cast<le_t<u16>&>(decrypted_header[0xC]) = res_crc;

		// Type 1
		reinterpret_cast<le_t<u16>&>(decrypted_header[0xE]) = 5;
		reinterpret_cast<le_t<u16>&>(decrypted_header[0xE]) = skylander_crc16(0xFFFF, decrypted_header.data(), 16);

		set_block(active, decrypted_header);
		set_block(active + 9, sub_header);
	}
}

bool skylander_dialog::is_initialized()
{
	std::lock_guard lock(g_skylander.sky_mutex);
	for (u32 index = 1; index < 0x10; index++)
	{
		for (u32 subdex = 0; subdex < (0x30 / sizeof(u64)); subdex++)
		{
			if (reinterpret_cast<const u64&>(g_skylander.sky_dump[(index * 0x40) + (subdex * sizeof(u64))]) != 0)
			{
				return true;
			}
		}
	}
	return false;
}

void skylander_dialog::new_skylander()
{
	const QString file_path = QFileDialog::getSaveFileName(this, tr("Create Skylander File"), cur_sky_file_path, tr("Skylander Object (*.sky);;"));
	if (file_path.isEmpty())
		return;

	if (g_skylander.sky_file)
		g_skylander.sky_file.close();

	g_skylander.sky_file.open(file_path.toStdString(), fs::read + fs::write + fs::create);
	if (!g_skylander.sky_file)
		return;

	cur_sky_file_path = file_path;

	{
		std::lock_guard lock(g_skylander.sky_mutex);
		memset(g_skylander.sky_dump, 0, 0x40 * 0x10);

		// Set the block permissions
		reinterpret_cast<le_t<u32>&>(g_skylander.sky_dump[0x36]) = 0x690F0F0F;
		for (u32 index = 1; index < 0x10; index++)
		{
			reinterpret_cast<le_t<u32>&>(g_skylander.sky_dump[(index * 0x40) + 0x36]) = 0x69080F7F;
		}

		reinterpret_cast<le_t<u16>&>(g_skylander.sky_dump[0x1E]) = skylander_crc16(0xFFFF, g_skylander.sky_dump, 0x1E);

		// On a new skylander everything is 0'd and no crc apart from the first one is set

		g_skylander.sky_reload = true;
	}

	g_skylander.sky_save();

	edit_curfile->setText(file_path);
	update_edits();
}

void skylander_dialog::load_skylander()
{
	const QString file_path = QFileDialog::getOpenFileName(this, tr("Select Skylander File"), cur_sky_file_path, tr("Skylander (*.sky);;"));
	if (file_path.isEmpty())
		return;

	if (g_skylander.sky_file)
		g_skylander.sky_file.close();

	g_skylander.sky_file.open(file_path.toStdString(), fs::read + fs::write);
	if (!g_skylander.sky_file)
		return;

	cur_sky_file_path = file_path;

	g_skylander.sky_load();

	edit_curfile->setText(file_path);
	update_edits();
}

void skylander_dialog::update_edits()
{
	// clang-format off
	auto widget_enabler = [&](bool status_noinit, bool status)
	{
		combo_skylist->setEnabled(status_noinit);
		edit_skyid->setEnabled(status_noinit);
		edit_skyxp->setEnabled(status);
		edit_skymoney->setEnabled(status);
		button_update->setEnabled(status_noinit);
	};
	// clang-format on

	if (!g_skylander.sky_file)
	{
		widget_enabler(false, false);
		return;
	}

	if (!is_initialized())
	{
		widget_enabler(true, false);
		std::lock_guard lock(g_skylander.sky_mutex);
		edit_skyid->setText(QString::number(reinterpret_cast<le_t<u16>&>(g_skylander.sky_dump[0x10])));
		return;
	}

	widget_enabler(true, true);
	{
		std::lock_guard lock(g_skylander.sky_mutex);
		edit_skyid->setText(QString::number(reinterpret_cast<le_t<u16>&>(g_skylander.sky_dump[0x10])));

		u8 active = get_active_block();

		std::array<u8, 16> decrypted;
		get_block(active, decrypted);
		u32 xp = reinterpret_cast<le_t<u32>&>(decrypted[0]) & 0xFFFFFF;
		edit_skyxp->setText(QString::number(xp));
		u16 money = reinterpret_cast<le_t<u16, 1>&>(decrypted[3]);
		edit_skymoney->setText(QString::number(money));
	}
}

void skylander_dialog::process_edits()
{
	bool cast_success = false;
	{
		std::lock_guard lock(g_skylander.sky_mutex);

		u16 skyID = edit_skyid->text().toInt(&cast_success);
		if (cast_success)
		{
			reinterpret_cast<le_t<u16>&>(g_skylander.sky_dump[0x10]) = skyID;
			reinterpret_cast<le_t<u16>&>(g_skylander.sky_dump[0])    = skyID;
		}

		reinterpret_cast<le_t<u16>&>(g_skylander.sky_dump[0x1E]) = skylander_crc16(0xFFFF, g_skylander.sky_dump, 0x1E);
	}

	if (is_initialized())
	{
		std::lock_guard lock(g_skylander.sky_mutex);
		u8 active = get_active_block();

		std::array<u8, 16> decrypted_header;
		get_block(active, decrypted_header);

		u32 old_xp    = reinterpret_cast<le_t<u32>&>(decrypted_header[0]) & 0xFFFFFF;
		u16 old_money = reinterpret_cast<le_t<u16, 1>&>(decrypted_header[3]);

		u32 xp = edit_skyxp->text().toInt(&cast_success);
		if (!cast_success)
			xp = old_xp;
		u32 money = edit_skymoney->text().toInt(&cast_success);
		if (!cast_success)
			money = old_money;

		memcpy(decrypted_header.data(), &xp, 3);
		reinterpret_cast<le_t<u16>&>(decrypted_header[3]) = money;

		set_block(active, decrypted_header);

		set_checksums();

		g_skylander.sky_reload = true;
	}

	g_skylander.sky_save();
}
