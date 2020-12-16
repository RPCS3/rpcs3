#include <mutex>
#include "Utilities/File.h"
#include "Crypto/md5.h"
#include "Crypto/aes.h"
#include "skylander_dialog.h"
#include "Emu/Io/Skylander.h"

#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QVBoxLayout>

skylander_dialog* skylander_dialog::inst = nullptr;

const std::map<const std::pair<const u16, const u16>, const std::string> list_skylanders = {
    {{0, 0}, "Whirlwind"},
    {{0, 280}, "Series 2 Whirlwind"},
    {{0, 540}, "Polar Whirlwind"},
    {{0, 1320}, "Horn Blast Whirlwind"},
    {{0, 4152}, "Eon's Elite Whirlwind"},
    {{1, 0}, "Sonic Boom"},
    {{1, 280}, "Series 2 Sonic Boom"},
    {{2, 0}, "Warnado"},
    {{2, 1570}, "LightCore Warnado"},
    {{3, 0}, "Lightning Rod"},
    {{3, 280}, "Series 2 Lightning Rod"},
    {{4, 0}, "Bash"},
    {{4, 280}, "Series 2 Bash"},
    {{5, 0}, "Terrafin"},
    {{5, 280}, "Series 2 Terrafin"},
    {{5, 1320}, "Knockout Terrafin"},
    {{5, 4152}, "Eon's Elite Terrafin"},
    {{6, 0}, "Dino Rang"},
    {{6, 4168}, "Eon's Elite Dino Rang"},
    {{7, 0}, "Prism Break"},
    {{7, 280}, "Series 2 Prism Break"},
    {{7, 1320}, "Hyper Beam Prism Break"},
    {{7, 1554}, "LightCore Prism Break"},
    {{8, 0}, "Sunburn"},
    {{9, 0}, "Eruptor"},
    {{9, 280}, "Series 2 Eruptor"},
    {{9, 556}, "Volcanic Eruptor"},
    {{9, 1320}, "Lava Barf Eruptor"},
    {{9, 1554}, "LightCore Eruptor"},
    {{9, 4152}, "Eon's Elite Eruptor"},
    {{10, 0}, "Ignitor"},
    {{10, 280}, "Series 2 Ignitor"},
    {{10, 796}, "Legendary Ignitor"},
    {{11, 0}, "Flameslinger"},
    {{11, 280}, "Series 2 Flameslinger"},
    {{12, 0}, "Zap"},
    {{12, 280}, "Series 2 Zap"},
    {{13, 0}, "Wham Shell"},
    {{13, 1570}, "LightCore Wham Shell"},
    {{14, 0}, "Gill Grunt"},
    {{14, 280}, "Series 2 Gill Grunt"},
    {{14, 1320}, "Anchors Away Gill Grunt"},
    {{14, 1336}, "Tidal Wave Gill Grunt"},
    {{14, 4152}, "Eon's Elite Gill Grunt"},
    {{15, 0}, "Slam Bam"},
    {{15, 280}, "Series 2 Slam Bam"},
    {{15, 796}, "Legendary Slam Bam"},
    {{15, 4168}, "Eon's Elite Slam Bam"},
    {{16, 0}, "Spyro"},
    {{16, 280}, "Series 2 Spyro"},
    {{16, 556}, "Dark Mega Ram Spyro"},
    {{16, 1320}, "Mega Ram Spyro"},
    {{16, 4152}, "Eon's Elite Spyro"},
    {{17, 0}, "Voodood"},
    {{17, 4168}, "Eon's Elite Voodood"},
    {{18, 0}, "Double Trouble"},
    {{18, 280}, "Series 2 Double Trouble"},
    {{18, 540}, "Royal Double Trouble"},
    {{19, 0}, "Trigger Happy"},
    {{19, 280}, "Series 2 Trigger Happy"},
    {{19, 556}, "Springtime Trigger Happy"},
    {{19, 4152}, "Eon's Elite Trigger Happy"},
    {{20, 0}, "Drobot"},
    {{20, 280}, "Series 2 Drobot"},
    {{20, 1554}, "LightCore Drobot"},
    {{21, 0}, "Drill Seargeant"},
    {{21, 280}, "Series 2 Drill Seargeant"},
    {{22, 0}, "Boomer"},
    {{22, 4168}, "Eon's Elite Boomer"},
    {{23, 0}, "Wrecking Ball"},
    {{23, 280}, "Series 2 Wrecking Ball"},
    {{24, 0}, "Camo"},
    {{24, 1320}, "Thorn Horn Camo"},
    {{25, 0}, "Zook"},
    {{25, 280}, "Series 2 Zook"},
    {{25, 4168}, "Eon's Elite Zook"},
    {{26, 0}, "Stealth Elf"},
    {{26, 280}, "Series 2 Stealth Elf"},
    {{26, 556}, "Dark Stealth Elf"},
    {{26, 796}, "Legendary Stealth Elf"},
    {{26, 1320}, "Ninja Stealth Elf"},
    {{26, 4152}, "Eon's Elite Stealth Elf"},
    {{27, 0}, "Stump Smash"},
    {{27, 280}, "Series 2 Stump Smash"},
    {{28, 0}, "Dark Spyro"},
    {{29, 0}, "Hex"},
    {{29, 280}, "Series 2 Hex"},
    {{29, 1554}, "LightCore Hex"},
    {{30, 0}, "Chop Chop"},
    {{30, 280}, "Series 2 Chop Chop"},
    {{30, 1320}, "Twin Blade Chop Chop"},
    {{30, 4152}, "Eon's Elite Chop Chop"},
    {{31, 0}, "Ghost Roaster"},
    {{31, 4168}, "Eon's Elite Ghost Roaster"},
    {{32, 0}, "Cynder"},
    {{32, 280}, "Series 2 Cynder"},
    {{32, 1320}, "Phantom Cynder"},
    {{100, 0}, "Jet Vac"},
    {{100, 788}, "Legendary Jet Vac"},
    {{100, 1320}, "Turbo Jet Vac"},
    {{100, 1336}, "Full Blast Jet Vac"},
    {{100, 1554}, "LightCore Jet Vac"},
    {{101, 0}, "Swarm"},
    {{102, 0}, "Crusher"},
    {{102, 534}, "Granite Crusher"},
    {{103, 0}, "Flashwing"},
    {{103, 532}, "Jade Flash Wing"},
    {{103, 1570}, "LightCore Flashwing"},
    {{104, 0}, "Hot Head"},
    {{105, 0}, "Hot Dog"},
    {{105, 532}, "Molten Hot Dog"},
    {{105, 1320}, "Fire Bone Hot Dog"},
    {{106, 0}, "Chill"},
    {{106, 790}, "Legendary Chill"},
    {{106, 1320}, "Blizzard Chill"},
    {{106, 1554}, "LightCore Chill"},
    {{107, 0}, "Thumpback"},
    {{108, 0}, "Pop Fizz"},
    {{108, 532}, "Punch Pop Fizz"},
    {{108, 572}, "Love Potion Pop Fizz"},
    {{108, 1320}, "Super Gulp Pop Fizz"},
    {{108, 1336}, "Fizzy Frenzy Pop Fizz"},
    {{108, 1554}, "LightCore Pop Fizz"},
    {{109, 0}, "Ninjini"},
    {{109, 534}, "Scarlet Ninjini"},
    {{110, 0}, "Bouncer"},
    {{110, 790}, "Legendary Bouncer"},
    {{111, 0}, "Sprocket"},
    {{111, 1320}, "Heavy Duty Sprocket"},
    {{112, 0}, "Tree Rex"},
    {{112, 534}, "Gnarly Tree Rex"},
    {{113, 0}, "Shroomboom"},
    {{113, 1336}, "Sure Shot Shroomboom"},
    {{113, 1554}, "LightCore Shroomboom"},
    {{114, 0}, "Eye Brawl"},
    {{115, 0}, "Fright Rider"},
    {{200, 0}, "Anvil Rain"},
    {{201, 0}, "Hidden Treasure"},
    {{201, 32}, "Platinum Hidden Treasure"},
    {{202, 0}, "Healing Elixir"},
    {{203, 0}, "Ghost Pirate Swords"},
    {{204, 0}, "Time Twist Hourglass"},
    {{205, 0}, "Sky Iron Shield"},
    {{206, 0}, "Winged Boots"},
    {{207, 0}, "Sparx the Dragonfly"},
    {{208, 0}, "Dragonfire Cannon"},
    {{208, 534}, "Golden Dragonfire Cannon"},
    {{209, 0}, "Scorpion Striker"},
    {{210, 560}, "Biter's Bane"},
    {{210, 2096}, "Sorcerous Skull"},
    {{210, 2864}, "Axe of Illusion"},
    {{210, 3632}, "Arcane Hourglass"},
    {{210, 4656}, "Spell Slapper"},
    {{210, 5168}, "Rune Rocket"},
    {{211, 304}, "Tidal Tiki"},
    {{211, 560}, "Wet Walter"},
    {{211, 1584}, "Flood Flask"},
    {{211, 1588}, "Legendary Flood Flask"},
    {{211, 1840}, "Soaking Staff"},
    {{211, 2864}, "Aqua Axe"},
    {{211, 5680}, "Frost Helm"},
    {{212, 816}, "Breezy Bird"},
    {{212, 1584}, "Drafty Decanter"},
    {{212, 3376}, "Tempest Timer"},
    {{212, 4144}, "Cloudy Cobra"},
    {{212, 4400}, "Storm Warning"},
    {{212, 6192}, "Cyclone Saber"},
    {{213, 1072}, "Spirit Sphere"},
    {{213, 1076}, "Legendary Spirit Sphere"},
    {{213, 2096}, "Spectral Skull"},
    {{213, 2100}, "Legendary Spectral Skull"},
    {{213, 2864}, "Haunted Hatchet"},
    {{213, 3120}, "Grim Gripper"},
    {{213, 4144}, "Spooky Snake"},
    {{213, 5936}, "Dream Piercer"},
    {{214, 48}, "Tech Totem"},
    {{214, 1840}, "Automatic Angel"},
    {{214, 2352}, "Factory Flower"},
    {{214, 3120}, "Grabbing Gadget"},
    {{214, 5680}, "Makers Mana"},
    {{214, 6704}, "Topsy Techy"},
    {{215, 1328}, "Eternal Flame"},
    {{215, 2352}, "Fire Flower"},
    {{215, 4400}, "Scorching Stopper"},
    {{215, 4656}, "Searing Spinner"},
    {{215, 5936}, "Spark Spear"},
    {{215, 6960}, "Blazing Belch"},
    {{216, 48}, "Banded Boulder"},
    {{216, 816}, "Rock Hawk"},
    {{216, 2608}, "Slag Hammer"},
    {{216, 3632}, "Dust Of Time"},
    {{216, 4912}, "Spinning Sandstorm"},
    {{216, 6704}, "Rubble Trouble"},
    {{217, 816}, "Oak Eagle"},
    {{217, 1328}, "Emerald Energy"},
    {{217, 2608}, "Weed Whacker"},
    {{217, 4144}, "Seed Serpent"},
    {{217, 6192}, "Jade Blade"},
    {{217, 6960}, "Shrub Shrieker"},
    {{218, 48}, "Dark Dagger"},
    {{218, 5168}, "Shadow Spider"},
    {{218, 6704}, "Ghastly Grimace"},
    {{219, 48}, "Shining Ship"},
    {{219, 3888}, "Heavenly Hawk"},
    {{219, 6960}, "Beam Scream"},
    {{220, 7728}, "Kaos Trap"},
    {{220, 7989}, "Ultimate Kaos Trap"},
    {{230, 0}, "Hand of Fate"},
    {{230, 820}, "Legendary Hand of Fate"},
    {{231, 0}, "Piggy Bank"},
    {{232, 0}, "Rocket Ram"},
    {{233, 0}, "Tiki Speaky"},
    {{300, 0}, "Dragon’s Peak"},
    {{301, 0}, "Empire of Ice"},
    {{302, 0}, "Pirate Seas"},
    {{303, 0}, "Darklight Crypt"},
    {{304, 0}, "Volcanic Vault"},
    {{305, 0}, "Mirror of Mystery"},
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
    {{453, 820}, "Legendary Blades"},
    {{454, 0}, "Wallop"},
    {{455, 0}, "Head Rush"},
    {{455, 564}, "Nitro Head Rush"},
    {{456, 0}, "Fist Bump"},
    {{457, 0}, "Rocky Roll"},
    {{458, 0}, "Wildfire"},
    {{458, 564}, "Dark Wildfire"},
    {{459, 0}, "Ka Boom"},
    {{460, 0}, "Trail Blazer"},
    {{461, 0}, "Torch"},
    {{462, 0}, "Snap Shot"},
    {{462, 564}, "Dark Snap Shot"},
    {{463, 0}, "Lob Star"},
    {{463, 564}, "Winterfest Lob-Star"},
    {{464, 0}, "Flip Wreck"},
    {{465, 0}, "Echo"},
    {{466, 0}, "Blastermind"},
    {{467, 0}, "Enigma"},
    {{468, 0}, "Deja Vu"},
    {{468, 820}, "Legendary Deja Vu"},
    {{469, 0}, "Cobra Candabra"},
    {{469, 564}, "King Cobra Cadabra"},
    {{470, 0}, "Jawbreaker"},
    {{470, 820}, "Legendary Jawbreaker"},
    {{471, 0}, "Gearshift"},
    {{472, 0}, "Chopper"},
    {{473, 0}, "Tread Head"},
    {{474, 0}, "Bushwack"},
    {{474, 820}, "Legendary Bushwack"},
    {{475, 0}, "Tuff Luck"},
    {{476, 0}, "Food Fight"},
    {{476, 564}, "Dark Food Fight"},
    {{477, 0}, "High Five"},
    {{478, 0}, "Krypt King"},
    {{478, 564}, "Nitro Krypt King"},
    {{479, 0}, "Short Cut"},
    {{480, 0}, "Bat Spin"},
    {{481, 0}, "Funny Bone"},
    {{482, 0}, "Knight Light"},
    {{483, 0}, "Spotlight"},
    {{484, 0}, "Knight Mare"},
    {{485, 0}, "Blackout"},
    {{502, 0}, "Bop"},
    {{505, 0}, "Terrabite"},
    {{506, 0}, "Breeze"},
    {{508, 0}, "Pet Vac"},
    {{508, 564}, "Power Punch Pet Vac"},
    {{507, 0}, "Weeruptor"},
    {{507, 564}, "Eggcellent Weeruptor"},
    {{509, 0}, "Small Fry"},
    {{510, 0}, "Drobit"},
    {{519, 0}, "Trigger Snappy"},
    {{526, 0}, "Whisper Elf"},
    {{540, 0}, "Barkley"},
    {{540, 564}, "Gnarly Barkley"},
    {{541, 0}, "Thumpling"},
    {{514, 0}, "Gill Runt"},
    {{542, 0}, "Mini-Jini"},
    {{503, 0}, "Spry"},
    {{504, 0}, "Hijinx"},
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
    {{3000, 0}, "Scratch"},
    {{3001, 0}, "Pop Thorn"},
    {{3002, 0}, "Slobber Tooth"},
    {{3002, 548}, "Dark Slobber Tooth"},
    {{3003, 0}, "Scorp"},
    {{3004, 0}, "Fryno"},
    {{3004, 1336}, "Hog Wild Fryno"},
    {{3005, 0}, "Smolderdash"},
    {{3005, 1570}, "LightCore Smolderdash"},
    {{3006, 0}, "Bumble Blast"},
    {{3006, 548}, "Jolly Bumble Blast"},
    {{3006, 1570}, "LightCore Bumble Blast"},
    {{3007, 0}, "Zoo Lou"},
    {{3007, 804}, "Legendary Zoo Lou"},
    {{3008, 0}, "Dune Bug"},
    {{3009, 0}, "Star Strike"},
    {{3009, 550}, "Enchanted Star Strike (LightCore)"},
    {{3009, 1570}, "LightCore Star Strike"},
    {{3010, 0}, "Countdown"},
    {{3010, 548}, "Kickoff Countdown"},
    {{3010, 1570}, "LightCore Countdown"},
    {{3011, 0}, "Wind Up"},
    {{3012, 0}, "Roller Brawl"},
    {{3013, 0}, "Grim Creeper"},
    {{3013, 806}, "Legendary Grim Creeper"},
    {{3013, 1570}, "LightCore Grim Creeper"},
    {{3014, 0}, "Rip Tide"},
    {{3015, 0}, "Punk Shock"},
    {{1000, 0}, "Boom Jet (Bottom)"},
    {{1001, 0}, "Free Ranger (Bottom)"},
    {{1001, 804}, "Legendary Free Ranger (Bottom)"},
    {{1002, 0}, "Rubble Rouser (Bottom)"},
    {{1003, 0}, "Doom Stone (Bottom)"},
    {{1004, 0}, "Blast Zone (Bottom)"},
    {{1004, 548}, "Dark Blast Zone (Bottom)"},
    {{1005, 0}, "Fire Kraken (Bottom)"},
    {{1005, 548}, "Jade Fire Kraken (Bottom)"},
    {{1006, 0}, "Stink Bomb (Bottom)"},
    {{1007, 0}, "Grilla Drilla (Bottom)"},
    {{1008, 0}, "Hoot Loop (Bottom)"},
    {{1008, 548}, "Enchanted Hoot Loop (Bottom)"},
    {{1009, 0}, "Trap Shadow (Bottom)"},
    {{1010, 0}, "Magna Charge (Bottom)"},
    {{1010, 548}, "Nitro Magna Charge (Bottom)"},
    {{1011, 0}, "Spy Rise (Bottom)"},
    {{1012, 0}, "Night Shift (Bottom)"},
    {{1012, 804}, "Legendary Night Shift (Bottom)"},
    {{1013, 0}, "Rattle Shake (Bottom)"},
    {{1013, 548}, "Quick Draw Rattle Shake (Bottom)"},
    {{1014, 0}, "Freeze Blade (Bottom)"},
    {{1014, 548}, "Nitro Freeze Blade (Bottom)"},
    {{1015, 0}, "Wash Buckler (Bottom)"},
    {{1015, 548}, "Dark Wash Buckler (Bottom)"},
    {{2000, 0}, "Boom Jet (Top)"},
    {{2001, 0}, "Free Ranger (Top)"},
    {{2001, 804}, "Legendary Free Ranger (Top)"},
    {{2002, 0}, "Rubble Rouser (Top)"},
    {{2003, 0}, "Doom Stone (Top)"},
    {{2004, 0}, "Blast Zone (Top)"},
    {{2004, 548}, "Dark Blast Zone (Top)"},
    {{2005, 0}, "Fire Kraken (Top)"},
    {{2005, 548}, "Jade Fire Kraken (Top)"},
    {{2006, 0}, "Stink Bomb (Top)"},
    {{2007, 0}, "Grilla Drilla (Top)"},
    {{2008, 0}, "Hoot Loop (Top)"},
    {{2008, 548}, "Enchanted Hoot Loop (Top)"},
    {{2009, 0}, "Trap Shadow (Top)"},
    {{2010, 0}, "Magna Charge (Top)"},
    {{2010, 548}, "Nitro Magna Charge (Top)"},
    {{2011, 0}, "Spy Rise (Top)"},
    {{2012, 0}, "Night Shift (Top)"},
    {{2012, 804}, "Legendary Night Shift (Top)"},
    {{2013, 0}, "Rattle Shake (Top)"},
    {{2013, 548}, "Quick Draw Rattle Shake (Top)"},
    {{2014, 0}, "Freeze Blade (Top)"},
    {{2014, 548}, "Nitro Freeze Blade (Top)"},
    {{2015, 0}, "Wash Buckler (Top)"},
    {{2015, 548}, "Dark Wash Buckler (Top)"},
    {{3200, 0}, "Battle Hammer"},
    {{3201, 0}, "Sky Diamond"},
    {{3202, 0}, "Platinum Sheep"},
    {{3203, 0}, "Groove Machine"},
    {{3204, 0}, "UFO Hat"},
    {{3300, 0}, "Sheep Wreck Island"},
    {{3301, 0}, "Tower of Time"},
    {{3302, 0}, "Fiery Forge"},
    {{3303, 0}, "Arkeyan Crossbow"},
    {{3220, 0}, "Jet Stream (Vehicle)"},
    {{3221, 0}, "Tomb Buggy (Vehicle)"},
    {{3222, 0}, "Reef Ripper (Vehicle)"},
    {{3223, 0}, "Burn Cycle (Vehicle)"},
    {{3224, 0}, "Hot Streak (Vehicle)"},
    {{3224, 580}, "Dark Hot Streak (Vehicle)"},
    {{3224, 1088}, "E3 Hot Streak"},
    {{3224, 7748}, "Golden Hot Streak"},
    {{3225, 0}, "Shark Tank (Vehicle)"},
    {{3226, 0}, "Thump Truck (Vehicle)"},
    {{3227, 0}, "Crypt Crusher (Vehicle)"},
    {{3228, 0}, "Stealth Stinger (Vehicle)"},
    {{3231, 0}, "Dive Bomber (Vehicle)"},
    {{3231, 580}, "Spring Ahead Dive Bomber"},
    {{3232, 0}, "Sky Slicer (Vehicle)"},
    //{{3233, 0}, "Clown Cruiser (Vehicle) (Nintendo Only)"},
    //{{3233, 580}, "Dark Clown Cruiser (Vehicle) (Nintendo Only)"},
    {{3234, 0}, "Gold Rusher (Vehicle)"},
    {{3234, 580}, "Power Blue Gold Rusher"},
    {{3235, 0}, "Shield Striker (Vehicle)"},
    {{3236, 0}, "Sun Runner (Vehicle)"},
    {{3236, 836}, "Legendary Sun Runner (Vehicle)"},
    {{3237, 0}, "Sea Shadow (Vehicle)"},
    {{3237, 580}, "Dark Sea Shadow (Vehicle)"},
    {{3238, 0}, "Splatter Splasher (Vehicle)"},
    {{3238, 580}, "Power Blue Splatter Splasher"},
    {{3239, 0}, "Soda Skimmer (Vehicle)"},
    {{3239, 580}, "Nitro Soda Skimmer (Vehicle)"},
    //{{3240, 0}, "Barrel Blaster (Vehicle) (Nintendo Only)"},
    //{{3240, 580}, "Dark Barrel Blaster (Vehicle) (Nintendo Only)"},
    {{3241, 0}, "Buzz Wing (Vehicle)"},
    {{3400, 0}, "Fiesta"},
    {{3400, 5445}, "Frightful Fiesta"},
    {{3401, 0}, "High Volt"},
    {{3402, 0}, "Splat"},
    {{3402, 581}, "Power Blue Splat"},
    {{3406, 0}, "Stormblade"},
    {{3411, 0}, "Smash Hit"},
    {{3411, 581}, "Steel Plated Smash Hit"},
    {{3412, 0}, "Spitfire"},
    {{3412, 581}, "Dark Spitfire"},
    {{3413, 0}, "Hurricane Jet Vac"},
    {{3413, 837}, "Legendary Hurricane Jet Vac"},
    {{3414, 0}, "Double Dare Trigger Happy"},
    {{3414, 581}, "Power Blue Double Dare Trigger Happy"},
    {{3415, 0}, "Super Shot Stealth Elf"},
    {{3415, 581}, "Dark Super Shot Stealth Elf"},
    {{3416, 0}, "Shark Shooter Terrafin"},
    {{3417, 0}, "Bone Bash Roller Brawl"},
    {{3417, 837}, "Legendary Bone Bash Roller Brawl"},
    {{3420, 0}, "Big Bubble Pop Fizz"},
    {{3420, 3653}, "Birthday Bash Big Bubble Pop Fizz"},
    {{3421, 0}, "Lava Lance Eruptor"},
    {{3422, 0}, "Deep Dive Gill Grunt"},
    //{{3423, 0}, "Turbo Charge Donkey Kong (Nintendo Only)"},
    //{{3423, 581}, "Dark Turbo Charge Donkey Kong (Nintendo Only)"},
    //{{3424, 0}, "Hammer Slam Bowser (Nintendo Only)"},
    //{{3424, 581}, "Dark Hammer Slam Bowser (Nintendo Only)"},
    {{3425, 0}, "Dive-Clops"},
    {{3425, 3653}, "Missile-Tow Dive-Clops"},
    {{3426, 0}, "Astroblast"},
    {{3426, 837}, "Legendary Astroblast"},
    {{3427, 0}, "Nightfall"},
    {{3428, 0}, "Thrillipede"},
    {{3428, 3397}, "Eggcited Thrillipede"},
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
