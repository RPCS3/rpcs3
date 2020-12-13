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
    {{0, 48}, "Whirlwind"},
    {{0, 6145}, "Series 2 Whirlwind"},
    {{0, 10245}, "Horn Blast Whirlwind"},
    {{1, 48}, "Sonic Boom"},
    {{1, 6145}, "Series 2 Sonic Boom"},
    {{2, 48}, "Warnado"},
    {{2, 4614}, "LightCore Warnado"},
    {{3, 48}, "Lightning Rod"},
    {{3, 6145}, "Series 2 Lightning Rod"},
    {{4, 48}, "Bash"},
    {{4, 6145}, "Series 2 Bash"},
    {{5, 48}, "Terrafin"},
    {{5, 6145}, "Series 2 Terrafin"},
    {{5, 10245}, "Knockout Terrafin"},
    {{6, 48}, "Dino Rang"},
    {{7, 48}, "Prism Break"},
    {{7, 4614}, "LightCore Prism Break"},
    {{7, 6145}, "Series 2 Prism Break"},
    {{7, 10245}, "Hyper Beam Prism Break"},
    {{8, 48}, "Sunburn"},
    {{9, 48}, "Eruptor"},
    {{9, 4614}, "LightCore Eruptor"},
    {{9, 6145}, "Series 2 Eruptor"},
    {{9, 10245}, "Lava Barf Eruptor"},
    {{10, 48}, "Ignitor"},
    {{10, 6145}, "Series 2 Ignitor"},
    {{11, 48}, "Flameslinger"},
    {{11, 6145}, "Series 2 Flameslinger"},
    {{12, 48}, "Zap"},
    {{12, 6145}, "Series 2 Zap"},
    {{13, 48}, "Wham Shell"},
    {{13, 4614}, "LightCore Wham Shell"},
    {{14, 48}, "Gill Grunt"},
    {{14, 6145}, "Series 2 Gill Grunt"},
    {{14, 10245}, "Anchors Away Gill Grunt"},
    {{15, 48}, "Slam Bam"},
    {{15, 6145}, "Series 2 Slam Bam"},
    {{16, 48}, "Spyro"},
    {{16, 6145}, "Series 2 Spyro"},
    {{16, 10245}, "Mega Ram Spyro"},
    {{16, 11266}, "Dark Mega Ram Spyro"},
    {{17, 48}, "Voodood"},
    {{18, 48}, "Double Trouble"},
    {{18, 6145}, "Series 2 Double Trouble"},
    {{19, 48}, "Trigger Happy"},
    {{19, 6145}, "Series 2 Trigger Happy"},
    {{20, 48}, "Drobot"},
    {{20, 4614}, "LightCore Drobot"},
    {{20, 6145}, "Series 2 Drobot"},
    {{21, 48}, "Drill Seargeant"},
    {{21, 6145}, "Series 2 Drill Seargeant"},
    {{22, 48}, "Boomer"},
    {{23, 48}, "Wrecking Ball"},
    {{23, 6145}, "Series 2 Wrecking Ball"},
    {{24, 48}, "Camo"},
    {{24, 10245}, "Thorn Horn Camo"},
    {{25, 48}, "Zook"},
    {{25, 6145}, "Series 2 Zook"},
    {{26, 48}, "Stealth Elf"},
    {{26, 6145}, "Series 2 Stealth Elf"},
    {{26, 10245}, "Ninja Stealth Elf"},
    {{26, 11266}, "Dark Stealth Elf"},
    {{27, 48}, "Stump Smash"},
    {{27, 6145}, "Series 2 Stump Smash"},
    {{28, 48}, "Dark Spyro"},
    {{28, 6145}, "Series 2 Dark Spyro"},
    {{29, 48}, "Hex"},
    {{29, 6145}, "Series 2 Hex"},
    {{30, 48}, "Chop Chop"},
    {{30, 6145}, "Series 2 Chop Chop"},
    {{30, 10245}, "Twin Blade Chop Chop"},
    {{31, 48}, "Ghost Roaster"},
    {{32, 48}, "Cynder"},
    {{32, 6145}, "Series 2 Cynder"},
    {{32, 10245}, "Phantom Cynder"},
    {{100, 48}, "Jet Vac"},
    {{100, 4614}, "LightCore Jet Vac"},
    {{100, 10245}, "Turbo Jet Vac"},
    {{101, 48}, "Swarm"},
    {{102, 48}, "Crusher"},
    {{103, 48}, "Flashwing"},
    {{103, 4614}, "LightCore Flashwing"},
    {{103, 13314}, "Jade Flash Wing"},
    {{104, 48}, "Hot Head"},
    {{105, 48}, "Hot Dog"},
    {{105, 10245}, "Fire Bone Hot Dog"},
    {{106, 48}, "Chill"},
    {{106, 4614}, "LightCore Chill"},
    {{106, 10245}, "Blizzard Chill"},
    {{107, 48}, "Thumpback"},
    {{108, 48}, "Pop Fizz"},
    {{108, 4614}, "LightCore Pop Fizz"},
    {{108, 10245}, "Super Gulp Pop Fizz"},
    {{108, 15362}, "Love Potion Pop Fizz"},
    {{109, 48}, "Ninjini"},
    {{109, 13314}, "Scarlet Ninjini"},
    {{110, 48}, "Bouncer"},
    {{111, 48}, "Sprocket"},
    {{111, 10245}, "Heavy Duty Sprocket"},
    {{112, 48}, "Tree Rex"},
    {{112, 13314}, "Gnarly Tree Rex"},
    {{113, 48}, "Shroomboom"},
    {{113, 4614}, "LightCore Shroomboom"},
    {{114, 48}, "Eye Brawl"},
    {{115, 48}, "Fright Rider"},
    {{200, 48}, "Anvil Rain"},
    {{201, 48}, "Hidden Treasure"},
    {{202, 48}, "Healing Elixir"},
    {{203, 48}, "Ghost Pirate Swords"},
    {{204, 48}, "Time Twist Hourglass"},
    {{205, 48}, "Sky Iron Shield"},
    {{206, 48}, "Winged Boots"},
    {{207, 48}, "Sparx the Dragonfly"},
    {{208, 48}, "Dragonfire Cannon"},
    {{209, 48}, "Scorpion Striker"},
    {{210, 12290}, "Biter's Bane"},
    {{210, 12296}, "Sorcerous Skull"},
    {{210, 12299}, "Axe of Illusion"},
    {{210, 12302}, "Arcane Hourglass"},
    {{210, 12306}, "Spell Slapper"},
    {{210, 12308}, "Rune Rocket"},
    {{211, 12289}, "Tidal Tiki"},
    {{211, 12290}, "Wet Walter"},
    {{211, 12294}, "Flood Flask"},
    {{211, 12295}, "Soaking Staff"},
    {{211, 12299}, "Aqua Axe"},
    {{211, 12310}, "Frost Helm"},
    {{212, 12291}, "Breezy Bird"},
    {{212, 12294}, "Drafty Decanter"},
    {{212, 12301}, "Tempest Timer"},
    {{212, 12304}, "Cloudy Cobra"},
    {{212, 12305}, "Storm Warning"},
    {{212, 12312}, "Cyclone Saber"},
    {{213, 12292}, "Spirit Sphere"},
    {{213, 12296}, "Spectral Skull"},
    {{213, 12299}, "Haunted Hatchet"},
    {{213, 12300}, "Grim Gripper"},
    {{213, 12304}, "Spooky Snake"},
    {{213, 12311}, "Dream Piercer"},
    {{214, 12288}, "Tech Totem"},
    {{214, 12295}, "Automatic Angel"},
    {{214, 12297}, "Factory Flower"},
    {{214, 12300}, "Grabbing Gadget"},
    {{214, 12310}, "Makers Mana"},
    {{214, 12314}, "Topsy Techy"},
    {{215, 12293}, "Eternal Flame"},
    {{215, 12297}, "Fire Flower"},
    {{215, 12305}, "Scorching Stopper"},
    {{215, 12306}, "Searing Spinner"},
    {{215, 12311}, "Spark Spear"},
    {{215, 12315}, "Blazing Belch"},
    {{216, 12288}, "Banded Boulder"},
    {{216, 12291}, "Rock Hawk"},
    {{216, 12298}, "Slag Hammer"},
    {{216, 12302}, "Dust Of Time"},
    {{216, 12307}, "Spinning Sandstorm"},
    {{216, 12314}, "Rubble Trouble"},
    {{217, 12291}, "Oak Eagle"},
    {{217, 12293}, "Emerald Energy"},
    {{217, 12298}, "Weed Whacker"},
    {{217, 12304}, "Seed Serpent"},
    {{217, 12312}, "Jade Blade"},
    {{217, 12315}, "Shrub Shrieker"},
    {{218, 12288}, "Dark Dagger"},
    {{218, 12308}, "Shadow Spider"},
    {{218, 12314}, "Ghastly Grimace"},
    {{219, 12288}, "Shining Ship"},
    {{219, 12303}, "Heavenly Hawk"},
    {{219, 12315}, "Beam Scream"},
    {{220, 12336}, "Kaos Trap"},
    {{220, 12337}, "Ultimate Kaos Trap"},
    {{230, 48}, "Hand of Fate"},
    {{231, 48}, "Piggy Bank"},
    {{232, 48}, "Rocket Ram"},
    {{233, 48}, "Tiki Speaky"},
    {{300, 48}, "Dragon’s Peak"},
    {{301, 48}, "Empire of Ice"},
    {{302, 48}, "Pirate Seas"},
    {{303, 48}, "Darklight Crypt"},
    {{304, 48}, "Volcanic Vault"},
    {{305, 48}, "Mirror of Mystery"},
    {{306, 48}, "Nightmare Express"},
    {{307, 48}, "Sunscraper Spire"},
    {{308, 48}, "Midnight Museum"},
    {{404, 48}, "Legendary Bash"},
    {{416, 48}, "Legendary Spyro"},
    {{419, 48}, "Legendary Trigger Happy"},
    {{430, 48}, "Legendary Chop Chop"},
    {{450, 48}, "Gusto"},
    {{451, 48}, "Thunderbolt"},
    {{452, 48}, "Fling Kong"},
    {{453, 48}, "Blades"},
    {{454, 48}, "Wallop"},
    {{455, 48}, "Head Rush"},
    {{456, 48}, "Fist Bump"},
    {{457, 48}, "Rocky Roll"},
    {{458, 48}, "Wildfire"},
    {{458, 11266}, "Dark Wildfire"},
    {{459, 48}, "Ka Boom"},
    {{460, 48}, "Trail Blazer"},
    {{461, 48}, "Torch"},
    {{462, 48}, "Snap Shot"},
    {{462, 11266}, "Dark Snap Shot"},
    {{463, 48}, "Lob Star"},
    {{464, 48}, "Flip Wreck"},
    {{465, 48}, "Echo"},
    {{466, 48}, "Blastermind"},
    {{467, 48}, "Enigma"},
    {{468, 48}, "Deja Vu"},
    {{469, 48}, "Cobra Candabra"},
    {{470, 48}, "Jawbreaker"},
    {{471, 48}, "Gearshift"},
    {{472, 48}, "Chopper"},
    {{473, 48}, "Tread Head"},
    {{474, 48}, "Bushwack"},
    {{475, 48}, "Tuff Luck"},
    {{476, 48}, "Food Fight"},
    {{476, 11266}, "Dark Food Fight"},
    {{477, 48}, "High Five"},
    {{478, 48}, "Krypt King"},
    {{479, 48}, "Short Cut"},
    {{480, 48}, "Bat Spin"},
    {{481, 48}, "Funny Bone"},
    {{482, 48}, "Knight Light"},
    {{483, 48}, "Spotlight"},
    {{484, 48}, "Knight Mare"},
    {{485, 48}, "Blackout"},
    {{502, 48}, "Bop"},
    {{505, 48}, "Terrabite"},
    {{506, 48}, "Breeze"},
    {{508, 48}, "Pet Vac"},
    {{507, 48}, "Weeruptor"},
    {{509, 48}, "Small Fry"},
    {{510, 48}, "Drobit"},
    {{519, 48}, "Trigger Snappy"},
    {{526, 48}, "Whisper Elf"},
    {{540, 48}, "Barkley"},
    {{540, 13314}, "Gnarly Barkley"},
    {{541, 48}, "Thumpling"},
    {{514, 48}, "Gill Runt"},
    {{542, 48}, "Mini-Jini"},
    {{503, 48}, "Spry"},
    {{504, 48}, "Hijinx"},
    {{543, 48}, "Eye Small"},
    {{601, 48}, "King Pen"},
    {{601, 11266}, "Dark King Pen"},
    {{602, 48}, "Tri-Tip"},
    {{603, 48}, "Chopscotch"},
    {{604, 48}, "Boom Bloom"},
    {{605, 48}, "Pit Boss"},
    {{606, 48}, "Barbella"},
    {{607, 48}, "Air Strike"},
    {{608, 48}, "Ember"},
    {{609, 48}, "Ambush"},
    {{610, 48}, "Dr. Krankcase"},
    {{611, 48}, "Hood Sickle"},
    {{612, 48}, "Tae Kwon Crow"},
    {{613, 48}, "Golden Queen"},
    {{613, 11266}, "Dark Golden Queen"},
    {{614, 48}, "Wolfgang"},
    {{614, 11266}, "Dark Wolfgang"},
    {{615, 48}, "Pain-Yatta"},
    {{616, 48}, "Mysticat"},
    {{617, 48}, "Starcast"},
    {{618, 48}, "Buckshot"},
    {{619, 48}, "Aurora"},
    {{620, 48}, "Flare Wolf"},
    {{621, 48}, "Chompy Mage"},
    {{622, 48}, "Bad Juju"},
    {{623, 48}, "Grave Clobber"},
    {{624, 48}, "Blaster-Tron"},
    {{625, 48}, "Ro-Bow"},
    {{626, 48}, "Chain Reaction"},
    {{627, 48}, "Kaos"},
    {{628, 48}, "Wild Storm"},
    {{629, 48}, "Tidepool"},
    {{630, 48}, "Crash Bandicoot"},
    {{631, 48}, "Dr. Neo Cortex"},
    {{3000, 48}, "Scratch"},
    {{3001, 48}, "Pop Thorn"},
    {{3002, 48}, "Slobber Tooth"},
    {{3002, 11266}, "Dark Slobber Tooth"},
    {{3003, 48}, "Scorp"},
    {{3004, 48}, "Fryno"},
    {{3005, 48}, "Smolderdash"},
    {{3005, 4614}, "LightCore Smolderdash"},
    {{3006, 48}, "Bumble Blast"},
    {{3006, 4614}, "LightCore Bumble Blast"},
    {{3007, 48}, "Zoo Lou"},
    {{3008, 48}, "Dune Bug"},
    {{3009, 48}, "Star Strike"},
    {{3009, 4614}, "LightCore Star Strike"},
    {{3010, 48}, "Countdown"},
    {{3010, 4614}, "LightCore Countdown"},
    {{3011, 48}, "Wind Up"},
    {{3012, 48}, "Roller Brawl"},
    {{3013, 48}, "Grim Creeper"},
    {{3013, 4614}, "LightCore Grim Creeper"},
    {{3014, 48}, "Rip Tide"},
    {{3015, 48}, "Punk Shock"},
    {{1000, 48}, "Boom Jet (Bottom)"},
    {{1001, 48}, "Free Ranger (Bottom)"},
    {{1002, 48}, "Rubble Rouser (Bottom)"},
    {{1003, 48}, "Doom Stone (Bottom)"},
    {{1004, 48}, "Blast Zone (Bottom)"},
    {{1004, 11266}, "Dark Blast Zone (Bottom)"},
    {{1005, 48}, "Fire Kraken (Bottom)"},
    {{1006, 48}, "Stink Bomb (Bottom)"},
    {{1007, 48}, "Grilla Drilla (Bottom)"},
    {{1008, 48}, "Hoot Loop (Bottom)"},
    {{1008, 9218}, "Enchanted Hoot Loop (Bottom)"},
    {{1009, 48}, "Trap Shadow (Bottom)"},
    {{1010, 48}, "Magna Charge (Bottom)"},
    {{1011, 48}, "Spy Rise (Bottom)"},
    {{1012, 48}, "Night Shift (Bottom)"},
    {{1013, 48}, "Rattle Shake (Bottom)"},
    {{1014, 48}, "Freeze Blade (Bottom)"},
    {{1015, 48}, "Wash Buckler (Bottom)"},
    {{1015, 11266}, "Dark Wash Buckler (Bottom)"},
    {{2000, 48}, "Boom Jet (Top)"},
    {{2001, 48}, "Free Ranger (Top)"},
    {{2002, 48}, "Rubble Rouser (Top)"},
    {{2003, 48}, "Doom Stone (Top)"},
    {{2004, 48}, "Blast Zone (Top)"},
    {{2004, 11266}, "Dark Blast Zone (Top)"},
    {{2005, 48}, "Fire Kraken (Top)"},
    {{2006, 48}, "Stink Bomb (Top)"},
    {{2007, 48}, "Grilla Drilla (Top)"},
    {{2008, 48}, "Hoot Loop (Top)"},
    {{2008, 9218}, "Enchanted Hoot Loop (Top)"},
    {{2009, 48}, "Trap Shadow (Top)"},
    {{2010, 48}, "Magna Charge (Top)"},
    {{2011, 48}, "Spy Rise (Top)"},
    {{2012, 48}, "Night Shift (Top)"},
    {{2013, 48}, "Rattle Shake (Top)"},
    {{2014, 48}, "Freeze Blade (Top)"},
    {{2015, 48}, "Wash Buckler (Top)"},
    {{2015, 11266}, "Dark Wash Buckler (Top)"},
    {{3220, 48}, "Jet Stream"},
    {{3221, 48}, "Tomb Buggy"},
    {{3222, 48}, "Reef Ripper"},
    {{3223, 48}, "Burn Cycle"},
    {{3224, 48}, "Hot Streak"},
    {{3225, 48}, "Shark Tank"},
    {{3226, 48}, "Thump Truck"},
    {{3227, 48}, "Crypt Crusher"},
    {{3228, 48}, "Stealth Stinger"},
    {{3231, 48}, "Dive Bomber"},
    {{3232, 48}, "Sky Slicer"},
    //{{3233, 48}, "Clown Cruiser (Nintendo Only)"},
    //{{3233, 11266}, "Dark Clown Cruiser (Nintendo Only)"},
    {{3234, 48}, "Gold Rusher"},
    {{3235, 48}, "Shield Striker"},
    {{3236, 48}, "Sun Runner"},
    {{3237, 48}, "Sea Shadow"},
    {{3237, 11266}, "Dark Sea Shadow"},
    {{3238, 48}, "Splatter Splasher"},
    {{3239, 48}, "Soda Skimmer"},
    //{{3240, 48}, "Barrel Blaster (Nintendo Only)"},
    //{{3240, 11266}, "Dark Barrel Blaster (Nintendo Only)"},
    {{3241, 48}, "Buzz Wing"},
    {{3400, 48}, "Fiesta"},
    {{3401, 48}, "High Volt"},
    {{3402, 48}, "Splat"},
    {{3406, 48}, "Stormblade"},
    {{3411, 48}, "Smash Hit"},
    {{3412, 48}, "Spitfire"},
    {{3412, 11266}, "Dark Spitfire"},
    {{3413, 48}, "Hurricane Jet Vac"},
    {{3414, 48}, "Double Dare Trigger Happy"},
    {{3415, 48}, "Super Shot Stealth Elf"},
    {{3415, 11266}, "Dark Super Shot Stealth Elf"},
    {{3416, 48}, "Shark Shooter Terrafin"},
    {{3417, 48}, "Bone Bash Roller Brawl"},
    {{3420, 48}, "Big Bubble Pop Fizz"},
    {{3421, 48}, "Lava Lance Eruptor"},
    {{3422, 48}, "Deep Dive Gill Grunt"},
    //{{3423, 48}, "Turbo Charge Donkey Kong (Nintendo Only)"},
    //{{3423, 11266}, "Dark Turbo Charge Donkey Kong (Nintendo Only)"},
    //{{3424, 48}, "Hammer Slam Bowser (Nintendo Only)"},
    //{{3424, 11266}, "Dark Hammer Slam Bowser (Nintendo Only)"},
    {{3425, 48}, "Dive-Clops"},
    {{3426, 48}, "Astroblast"},
    {{3427, 48}, "Nightfall"},
    {{3428, 48}, "Thrillipede"},
    {{3500, 48}, "Sky Trophy"},
    {{3501, 48}, "Land Trophy"},
    {{3502, 48}, "Sea Trophy"},
    {{3503, 48}, "Kaos Trophy"},
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
