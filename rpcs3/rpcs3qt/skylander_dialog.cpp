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
    {{0, 0x0000}, "Whirlwind"},
    {{0, 0x1801}, "Series 2 Whirlwind"},
    {{0, 0x1C02}, "Polar Whirlwind"},
    {{0, 0x2805}, "Horn Blast Whirlwind"},
    {{0, 0x3810}, "Eon's Elite Whirlwind"},
    {{1, 0x0000}, "Sonic Boom"},
    {{1, 0x1801}, "Series 2 Sonic Boom"},
    {{2, 0x0000}, "Warnado"},
    {{2, 0x2206}, "LightCore Warnado"},
    {{3, 0x0000}, "Lightning Rod"},
    {{3, 0x1801}, "Series 2 Lightning Rod"},
    {{4, 0x0000}, "Bash"},
    {{4, 0x1801}, "Series 2 Bash"},
    {{5, 0x0000}, "Terrafin"},
    {{5, 0x1801}, "Series 2 Terrafin"},
    {{5, 0x2805}, "Knockout Terrafin"},
    {{5, 0x3810}, "Eon's Elite Terrafin"},
    {{6, 0x0000}, "Dino Rang"},
    {{6, 0x4810}, "Eon's Elite Dino Rang"},
    {{7, 0x0000}, "Prism Break"},
    {{7, 0x1801}, "Series 2 Prism Break"},
    {{7, 0x2805}, "Hyper Beam Prism Break"},
    {{7, 0x1206}, "LightCore Prism Break"},
    {{8, 0x0000}, "Sunburn"},
    {{9, 0x0000}, "Eruptor"},
    {{9, 0x1801}, "Series 2 Eruptor"},
    {{9, 0x2C02}, "Volcanic Eruptor"},
    {{9, 0x2805}, "Lava Barf Eruptor"},
    {{9, 0x1206}, "LightCore Eruptor"},
    {{9, 0x3810}, "Eon's Elite Eruptor"},
    {{10, 0x0000}, "Ignitor"},
    {{10, 0x1801}, "Series 2 Ignitor"},
    {{10, 0x1C03}, "Legendary Ignitor"},
    {{11, 0x0000}, "Flameslinger"},
    {{11, 0x1801}, "Series 2 Flameslinger"},
    {{12, 0x0000}, "Zap"},
    {{12, 0x1801}, "Series 2 Zap"},
    {{13, 0x0000}, "Wham Shell"},
    {{13, 0x2206}, "LightCore Wham Shell"},
    {{14, 0x0000}, "Gill Grunt"},
    {{14, 0x1801}, "Series 2 Gill Grunt"},
    {{14, 0x2805}, "Anchors Away Gill Grunt"},
    {{14, 0x3805}, "Tidal Wave Gill Grunt"},
    {{14, 0x3810}, "Eon's Elite Gill Grunt"},
    {{15, 0x0000}, "Slam Bam"},
    {{15, 0x1801}, "Series 2 Slam Bam"},
    {{15, 0x1C03}, "Legendary Slam Bam"},
    {{15, 0x4810}, "Eon's Elite Slam Bam"},
    {{16, 0x0000}, "Spyro"},
    {{16, 0x1801}, "Series 2 Spyro"},
    {{16, 0x2C02}, "Dark Mega Ram Spyro"},
    {{16, 0x2805}, "Mega Ram Spyro"},
    {{16, 0x3810}, "Eon's Elite Spyro"},
    {{17, 0x0000}, "Voodood"},
    {{17, 0x4810}, "Eon's Elite Voodood"},
    {{18, 0x0000}, "Double Trouble"},
    {{18, 0x1801}, "Series 2 Double Trouble"},
    {{18, 0x1C02}, "Royal Double Trouble"},
    {{19, 0x0000}, "Trigger Happy"},
    {{19, 0x1801}, "Series 2 Trigger Happy"},
    {{19, 0x2C02}, "Springtime Trigger Happy"},
    {{19, 0x2805}, "Big Bang Trigger Happy"},
    {{19, 0x3810}, "Eon's Elite Trigger Happy"},
    {{20, 0x0000}, "Drobot"},
    {{20, 0x1801}, "Series 2 Drobot"},
    {{20, 0x1206}, "LightCore Drobot"},
    {{21, 0x0000}, "Drill Seargeant"},
    {{21, 0x1801}, "Series 2 Drill Seargeant"},
    {{22, 0x0000}, "Boomer"},
    {{22, 0x4810}, "Eon's Elite Boomer"},
    {{23, 0x0000}, "Wrecking Ball"},
    {{23, 0x1801}, "Series 2 Wrecking Ball"},
    {{24, 0x0000}, "Camo"},
    {{24, 0x2805}, "Thorn Horn Camo"},
    {{25, 0x0000}, "Zook"},
    {{25, 0x1801}, "Series 2 Zook"},
    {{25, 0x4810}, "Eon's Elite Zook"},
    {{26, 0x0000}, "Stealth Elf"},
    {{26, 0x1801}, "Series 2 Stealth Elf"},
    {{26, 0x2C02}, "Dark Stealth Elf"},
    {{26, 0x1C03}, "Legendary Stealth Elf"},
    {{26, 0x2805}, "Ninja Stealth Elf"},
    {{26, 0x3810}, "Eon's Elite Stealth Elf"},
    {{27, 0x0000}, "Stump Smash"},
    {{27, 0x1801}, "Series 2 Stump Smash"},
    {{28, 0x0000}, "Dark Spyro"},
    {{29, 0x0000}, "Hex"},
    {{29, 0x1801}, "Series 2 Hex"},
    {{29, 0x1206}, "LightCore Hex"},
    {{30, 0x0000}, "Chop Chop"},
    {{30, 0x1801}, "Series 2 Chop Chop"},
    {{30, 0x2805}, "Twin Blade Chop Chop"},
    {{30, 0x3810}, "Eon's Elite Chop Chop"},
    {{31, 0x0000}, "Ghost Roaster"},
    {{31, 0x4810}, "Eon's Elite Ghost Roaster"},
    {{32, 0x0000}, "Cynder"},
    {{32, 0x1801}, "Series 2 Cynder"},
    {{32, 0x2805}, "Phantom Cynder"},
    {{100, 0x0000}, "Jet Vac"},
    {{100, 0x1403}, "Legendary Jet Vac"},
    {{100, 0x2805}, "Turbo Jet Vac"},
    {{100, 0x3805}, "Full Blast Jet Vac"},
    {{100, 0x1206}, "LightCore Jet Vac"},
    {{101, 0x0000}, "Swarm"},
    {{102, 0x0000}, "Crusher"},
    {{102, 0x1602}, "Granite Crusher"},
    {{103, 0x0000}, "Flashwing"},
    {{103, 0x1402}, "Jade Flash Wing"},
    {{103, 0x2206}, "LightCore Flashwing"},
    {{104, 0x0000}, "Hot Head"},
    {{105, 0x0000}, "Hot Dog"},
    {{105, 0x1402}, "Molten Hot Dog"},
    {{105, 0x2805}, "Fire Bone Hot Dog"},
    {{106, 0x0000}, "Chill"},
    {{106, 0x1603}, "Legendary Chill"},
    {{106, 0x2805}, "Blizzard Chill"},
    {{106, 0x1206}, "LightCore Chill"},
    {{107, 0x0000}, "Thumpback"},
    {{108, 0x0000}, "Pop Fizz"},
    {{108, 0x1402}, "Punch Pop Fizz"},
    {{108, 0x3C02}, "Love Potion Pop Fizz"},
    {{108, 0x2805}, "Super Gulp Pop Fizz"},
    {{108, 0x3805}, "Fizzy Frenzy Pop Fizz"},
    {{108, 0x1206}, "LightCore Pop Fizz"},
    {{109, 0x0000}, "Ninjini"},
    {{109, 0x1602}, "Scarlet Ninjini"},
    {{110, 0x0000}, "Bouncer"},
    {{110, 0x1603}, "Legendary Bouncer"},
    {{111, 0x0000}, "Sprocket"},
    {{111, 0x2805}, "Heavy Duty Sprocket"},
    {{112, 0x0000}, "Tree Rex"},
    {{112, 0x1602}, "Gnarly Tree Rex"},
    {{113, 0x0000}, "Shroomboom"},
    {{113, 0x3805}, "Sure Shot Shroomboom"},
    {{113, 0x1206}, "LightCore Shroomboom"},
    {{114, 0x0000}, "Eye Brawl"},
    {{115, 0x0000}, "Fright Rider"},
    {{200, 0x0000}, "Anvil Rain"},
    {{201, 0x0000}, "Hidden Treasure"},
    {{201, 0x2000}, "Platinum Hidden Treasure"},
    {{202, 0x0000}, "Healing Elixir"},
    {{203, 0x0000}, "Ghost Pirate Swords"},
    {{204, 0x0000}, "Time Twist Hourglass"},
    {{205, 0x0000}, "Sky Iron Shield"},
    {{206, 0x0000}, "Winged Boots"},
    {{207, 0x0000}, "Sparx the Dragonfly"},
    {{208, 0x0000}, "Dragonfire Cannon"},
    {{208, 0x1602}, "Golden Dragonfire Cannon"},
    {{209, 0x0000}, "Scorpion Striker"},
    {{210, 0x3002}, "Biter's Bane"},
    {{210, 0x3008}, "Sorcerous Skull"},
    {{210, 0x300B}, "Axe of Illusion"},
    {{210, 0x300E}, "Arcane Hourglass"},
    {{210, 0x3012}, "Spell Slapper"},
    {{210, 0x3014}, "Rune Rocket"},
    {{211, 0x3001}, "Tidal Tiki"},
    {{211, 0x3002}, "Wet Walter"},
    {{211, 0x3006}, "Flood Flask"},
    {{211, 0x3406}, "Legendary Flood Flask"},
    {{211, 0x3007}, "Soaking Staff"},
    {{211, 0x300B}, "Aqua Axe"},
    {{211, 0x3016}, "Frost Helm"},
    {{212, 0x3003}, "Breezy Bird"},
    {{212, 0x3006}, "Drafty Decanter"},
    {{212, 0x300D}, "Tempest Timer"},
    {{212, 0x3010}, "Cloudy Cobra"},
    {{212, 0x3011}, "Storm Warning"},
    {{212, 0x3018}, "Cyclone Saber"},
    {{213, 0x3004}, "Spirit Sphere"},
    {{213, 0x3404}, "Legendary Spirit Sphere"},
    {{213, 0x3008}, "Spectral Skull"},
    {{213, 0x3408}, "Legendary Spectral Skull"},
    {{213, 0x300B}, "Haunted Hatchet"},
    {{213, 0x300C}, "Grim Gripper"},
    {{213, 0x3010}, "Spooky Snake"},
    {{213, 0x3017}, "Dream Piercer"},
    {{214, 0x3000}, "Tech Totem"},
    {{214, 0x3007}, "Automatic Angel"},
    {{214, 0x3009}, "Factory Flower"},
    {{214, 0x300C}, "Grabbing Gadget"},
    {{214, 0x3016}, "Makers Mana"},
    {{214, 0x301A}, "Topsy Techy"},
    {{215, 0x3005}, "Eternal Flame"},
    {{215, 0x3009}, "Fire Flower"},
    {{215, 0x3011}, "Scorching Stopper"},
    {{215, 0x3012}, "Searing Spinner"},
    {{215, 0x3017}, "Spark Spear"},
    {{215, 0x301B}, "Blazing Belch"},
    {{216, 0x3000}, "Banded Boulder"},
    {{216, 0x3003}, "Rock Hawk"},
    {{216, 0x300A}, "Slag Hammer"},
    {{216, 0x300E}, "Dust Of Time"},
    {{216, 0x3013}, "Spinning Sandstorm"},
    {{216, 0x301A}, "Rubble Trouble"},
    {{217, 0x3003}, "Oak Eagle"},
    {{217, 0x3005}, "Emerald Energy"},
    {{217, 0x300A}, "Weed Whacker"},
    {{217, 0x3010}, "Seed Serpent"},
    {{217, 0x3018}, "Jade Blade"},
    {{217, 0x301B}, "Shrub Shrieker"},
    {{218, 0x3000}, "Dark Dagger"},
    {{218, 0x3014}, "Shadow Spider"},
    {{218, 0x301A}, "Ghastly Grimace"},
    {{219, 0x3000}, "Shining Ship"},
    {{219, 0x300F}, "Heavenly Hawk"},
    {{219, 0x301B}, "Beam Scream"},
    {{220, 0x301E}, "Kaos Trap"},
    {{220, 0x351F}, "Ultimate Kaos Trap"},
    {{230, 0x0000}, "Hand of Fate"},
    {{230, 0x3403}, "Legendary Hand of Fate"},
    {{231, 0x0000}, "Piggy Bank"},
    {{232, 0x0000}, "Rocket Ram"},
    {{233, 0x0000}, "Tiki Speaky"},
    {{300, 0x0000}, "Dragon’s Peak"},
    {{301, 0x0000}, "Empire of Ice"},
    {{302, 0x0000}, "Pirate Seas"},
    {{303, 0x0000}, "Darklight Crypt"},
    {{304, 0x0000}, "Volcanic Vault"},
    {{305, 0x0000}, "Mirror of Mystery"},
    {{306, 0x0000}, "Nightmare Express"},
    {{307, 0x0000}, "Sunscraper Spire"},
    {{308, 0x0000}, "Midnight Museum"},
    {{404, 0x0000}, "Legendary Bash"},
    {{416, 0x0000}, "Legendary Spyro"},
    {{419, 0x0000}, "Legendary Trigger Happy"},
    {{430, 0x0000}, "Legendary Chop Chop"},
    {{450, 0x0000}, "Gusto"},
    {{451, 0x0000}, "Thunderbolt"},
    {{452, 0x0000}, "Fling Kong"},
    {{453, 0x0000}, "Blades"},
    {{453, 0x3403}, "Legendary Blades"},
    {{454, 0x0000}, "Wallop"},
    {{455, 0x0000}, "Head Rush"},
    {{455, 0x3402}, "Nitro Head Rush"},
    {{456, 0x0000}, "Fist Bump"},
    {{457, 0x0000}, "Rocky Roll"},
    {{458, 0x0000}, "Wildfire"},
    {{458, 0x3402}, "Dark Wildfire"},
    {{459, 0x0000}, "Ka Boom"},
    {{460, 0x0000}, "Trail Blazer"},
    {{461, 0x0000}, "Torch"},
    {{462, 0x0000}, "Snap Shot"},
    {{462, 0x3402}, "Dark Snap Shot"},
    {{463, 0x0000}, "Lob Star"},
    {{463, 0x3402}, "Winterfest Lob-Star"},
    {{464, 0x0000}, "Flip Wreck"},
    {{465, 0x0000}, "Echo"},
    {{466, 0x0000}, "Blastermind"},
    {{467, 0x0000}, "Enigma"},
    {{468, 0x0000}, "Deja Vu"},
    {{468, 0x3403}, "Legendary Deja Vu"},
    {{469, 0x0000}, "Cobra Candabra"},
    {{469, 0x3402}, "King Cobra Cadabra"},
    {{470, 0x0000}, "Jawbreaker"},
    {{470, 0x3403}, "Legendary Jawbreaker"},
    {{471, 0x0000}, "Gearshift"},
    {{472, 0x0000}, "Chopper"},
    {{473, 0x0000}, "Tread Head"},
    {{474, 0x0000}, "Bushwack"},
    {{474, 0x3403}, "Legendary Bushwack"},
    {{475, 0x0000}, "Tuff Luck"},
    {{476, 0x0000}, "Food Fight"},
    {{476, 0x3402}, "Dark Food Fight"},
    {{477, 0x0000}, "High Five"},
    {{478, 0x0000}, "Krypt King"},
    {{478, 0x3402}, "Nitro Krypt King"},
    {{479, 0x0000}, "Short Cut"},
    {{480, 0x0000}, "Bat Spin"},
    {{481, 0x0000}, "Funny Bone"},
    {{482, 0x0000}, "Knight Light"},
    {{483, 0x0000}, "Spotlight"},
    {{484, 0x0000}, "Knight Mare"},
    {{485, 0x0000}, "Blackout"},
    {{502, 0x0000}, "Bop"},
    {{505, 0x0000}, "Terrabite"},
    {{506, 0x0000}, "Breeze"},
    {{508, 0x0000}, "Pet Vac"},
    {{508, 0x3402}, "Power Punch Pet Vac"},
    {{507, 0x0000}, "Weeruptor"},
    {{507, 0x3402}, "Eggcellent Weeruptor"},
    {{509, 0x0000}, "Small Fry"},
    {{510, 0x0000}, "Drobit"},
    {{519, 0x0000}, "Trigger Snappy"},
    {{526, 0x0000}, "Whisper Elf"},
    {{540, 0x0000}, "Barkley"},
    {{540, 0x3402}, "Gnarly Barkley"},
    {{541, 0x0000}, "Thumpling"},
    {{514, 0x0000}, "Gill Runt"},
    {{542, 0x0000}, "Mini-Jini"},
    {{503, 0x0000}, "Spry"},
    {{504, 0x0000}, "Hijinx"},
    {{543, 0x0000}, "Eye Small"},
    {{601, 0x0000}, "King Pen"},
    {{602, 0x0000}, "Tri-Tip"},
    {{603, 0x0000}, "Chopscotch"},
    {{604, 0x0000}, "Boom Bloom"},
    {{605, 0x0000}, "Pit Boss"},
    {{606, 0x0000}, "Barbella"},
    {{607, 0x0000}, "Air Strike"},
    {{608, 0x0000}, "Ember"},
    {{609, 0x0000}, "Ambush"},
    {{610, 0x0000}, "Dr. Krankcase"},
    {{611, 0x0000}, "Hood Sickle"},
    {{612, 0x0000}, "Tae Kwon Crow"},
    {{613, 0x0000}, "Golden Queen"},
    {{614, 0x0000}, "Wolfgang"},
    {{615, 0x0000}, "Pain-Yatta"},
    {{616, 0x0000}, "Mysticat"},
    {{617, 0x0000}, "Starcast"},
    {{618, 0x0000}, "Buckshot"},
    {{619, 0x0000}, "Aurora"},
    {{620, 0x0000}, "Flare Wolf"},
    {{621, 0x0000}, "Chompy Mage"},
    {{622, 0x0000}, "Bad Juju"},
    {{623, 0x0000}, "Grave Clobber"},
    {{624, 0x0000}, "Blaster-Tron"},
    {{625, 0x0000}, "Ro-Bow"},
    {{626, 0x0000}, "Chain Reaction"},
    {{627, 0x0000}, "Kaos"},
    {{628, 0x0000}, "Wild Storm"},
    {{629, 0x0000}, "Tidepool"},
    {{630, 0x0000}, "Crash Bandicoot"},
    {{631, 0x0000}, "Dr. Neo Cortex"},
    {{1000, 0x0000}, "Boom Jet (Bottom)"},
    {{1001, 0x0000}, "Free Ranger (Bottom)"},
    {{1001, 0x2403}, "Legendary Free Ranger (Bottom)"},
    {{1002, 0x0000}, "Rubble Rouser (Bottom)"},
    {{1003, 0x0000}, "Doom Stone (Bottom)"},
    {{1004, 0x0000}, "Blast Zone (Bottom)"},
    {{1004, 0x2402}, "Dark Blast Zone (Bottom)"},
    {{1005, 0x0000}, "Fire Kraken (Bottom)"},
    {{1005, 0x2402}, "Jade Fire Kraken (Bottom)"},
    {{1006, 0x0000}, "Stink Bomb (Bottom)"},
    {{1007, 0x0000}, "Grilla Drilla (Bottom)"},
    {{1008, 0x0000}, "Hoot Loop (Bottom)"},
    {{1008, 0x2402}, "Enchanted Hoot Loop (Bottom)"},
    {{1009, 0x0000}, "Trap Shadow (Bottom)"},
    {{1010, 0x0000}, "Magna Charge (Bottom)"},
    {{1010, 0x2402}, "Nitro Magna Charge (Bottom)"},
    {{1011, 0x0000}, "Spy Rise (Bottom)"},
    {{1012, 0x0000}, "Night Shift (Bottom)"},
    {{1012, 0x2403}, "Legendary Night Shift (Bottom)"},
    {{1013, 0x0000}, "Rattle Shake (Bottom)"},
    {{1013, 0x2402}, "Quick Draw Rattle Shake (Bottom)"},
    {{1014, 0x0000}, "Freeze Blade (Bottom)"},
    {{1014, 0x2402}, "Nitro Freeze Blade (Bottom)"},
    {{1015, 0x0000}, "Wash Buckler (Bottom)"},
    {{1015, 0x2402}, "Dark Wash Buckler (Bottom)"},
    {{2000, 0x0000}, "Boom Jet (Top)"},
    {{2001, 0x0000}, "Free Ranger (Top)"},
    {{2001, 0x2403}, "Legendary Free Ranger (Top)"},
    {{2002, 0x0000}, "Rubble Rouser (Top)"},
    {{2003, 0x0000}, "Doom Stone (Top)"},
    {{2004, 0x0000}, "Blast Zone (Top)"},
    {{2004, 0x2402}, "Dark Blast Zone (Top)"},
    {{2005, 0x0000}, "Fire Kraken (Top)"},
    {{2005, 0x2402}, "Jade Fire Kraken (Top)"},
    {{2006, 0x0000}, "Stink Bomb (Top)"},
    {{2007, 0x0000}, "Grilla Drilla (Top)"},
    {{2008, 0x0000}, "Hoot Loop (Top)"},
    {{2008, 0x2402}, "Enchanted Hoot Loop (Top)"},
    {{2009, 0x0000}, "Trap Shadow (Top)"},
    {{2010, 0x0000}, "Magna Charge (Top)"},
    {{2010, 0x2402}, "Nitro Magna Charge (Top)"},
    {{2011, 0x0000}, "Spy Rise (Top)"},
    {{2012, 0x0000}, "Night Shift (Top)"},
    {{2012, 0x2403}, "Legendary Night Shift (Top)"},
    {{2013, 0x0000}, "Rattle Shake (Top)"},
    {{2013, 0x2402}, "Quick Draw Rattle Shake (Top)"},
    {{2014, 0x0000}, "Freeze Blade (Top)"},
    {{2014, 0x2402}, "Nitro Freeze Blade (Top)"},
    {{2015, 0x0000}, "Wash Buckler (Top)"},
    {{2015, 0x2402}, "Dark Wash Buckler (Top)"},
    {{3000, 0x0000}, "Scratch"},
    {{3001, 0x0000}, "Pop Thorn"},
    {{3002, 0x0000}, "Slobber Tooth"},
    {{3002, 0x2402}, "Dark Slobber Tooth"},
    {{3003, 0x0000}, "Scorp"},
    {{3004, 0x0000}, "Fryno"},
    {{3004, 0x3805}, "Hog Wild Fryno"},
    {{3005, 0x0000}, "Smolderdash"},
    {{3005, 0x2206}, "LightCore Smolderdash"},
    {{3006, 0x0000}, "Bumble Blast"},
    {{3006, 0x2402}, "Jolly Bumble Blast"},
    {{3006, 0x2206}, "LightCore Bumble Blast"},
    {{3007, 0x0000}, "Zoo Lou"},
    {{3007, 0x2403}, "Legendary Zoo Lou"},
    {{3008, 0x0000}, "Dune Bug"},
    {{3009, 0x0000}, "Star Strike"},
    {{3009, 0x2602}, "Enchanted Star Strike"},
    {{3009, 0x2206}, "LightCore Star Strike"},
    {{3010, 0x0000}, "Countdown"},
    {{3010, 0x2402}, "Kickoff Countdown"},
    {{3010, 0x2206}, "LightCore Countdown"},
    {{3011, 0x0000}, "Wind Up"},
    {{3012, 0x0000}, "Roller Brawl"},
    {{3013, 0x0000}, "Grim Creeper"},
    {{3013, 0x2603}, "Legendary Grim Creeper"},
    {{3013, 0x2206}, "LightCore Grim Creeper"},
    {{3014, 0x0000}, "Rip Tide"},
    {{3015, 0x0000}, "Punk Shock"},
    {{3200, 0x0000}, "Battle Hammer"},
    {{3201, 0x0000}, "Sky Diamond"},
    {{3202, 0x0000}, "Platinum Sheep"},
    {{3203, 0x0000}, "Groove Machine"},
    {{3204, 0x0000}, "UFO Hat"},
    {{3300, 0x0000}, "Sheep Wreck Island"},
    {{3301, 0x0000}, "Tower of Time"},
    {{3302, 0x0000}, "Fiery Forge"},
    {{3303, 0x0000}, "Arkeyan Crossbow"},
    {{3220, 0x0000}, "Jet Stream"},
    {{3221, 0x0000}, "Tomb Buggy"},
    {{3222, 0x0000}, "Reef Ripper"},
    {{3223, 0x0000}, "Burn Cycle"},
    {{3224, 0x0000}, "Hot Streak"},
    {{3224, 0x4402}, "Dark Hot Streak"},
    {{3224, 0x4004}, "E3 Hot Streak"},
    {{3224, 0x441E}, "Golden Hot Streak"},
    {{3225, 0x0000}, "Shark Tank"},
    {{3226, 0x0000}, "Thump Truck"},
    {{3227, 0x0000}, "Crypt Crusher"},
    {{3228, 0x0000}, "Stealth Stinger"},
    {{3231, 0x0000}, "Dive Bomber"},
    {{3231, 0x4402}, "Spring Ahead Dive Bomber"},
    {{3232, 0x0000}, "Sky Slicer"},
    //{{3233, 0x0000}, "Clown Cruiser (Nintendo Only)"},
    //{{3233, 0x4402}, "Dark Clown Cruiser (Nintendo Only)"},
    {{3234, 0x0000}, "Gold Rusher"},
    {{3234, 0x4402}, "Power Blue Gold Rusher"},
    {{3235, 0x0000}, "Shield Striker"},
    {{3236, 0x0000}, "Sun Runner"},
    {{3236, 0x4403}, "Legendary Sun Runner"},
    {{3237, 0x0000}, "Sea Shadow"},
    {{3237, 0x4402}, "Dark Sea Shadow"},
    {{3238, 0x0000}, "Splatter Splasher"},
    {{3238, 0x4402}, "Power Blue Splatter Splasher"},
    {{3239, 0x0000}, "Soda Skimmer"},
    {{3239, 0x4402}, "Nitro Soda Skimmer"},
    //{{3240, 0x0000}, "Barrel Blaster (Nintendo Only)"},
    //{{3240, 0x4402}, "Dark Barrel Blaster (Nintendo Only)"},
    {{3241, 0x0000}, "Buzz Wing"},
    {{3400, 0x0000}, "Fiesta"},
    {{3400, 0x4515}, "Frightful Fiesta"},
    {{3401, 0x0000}, "High Volt"},
    {{3402, 0x0000}, "Splat"},
    {{3402, 0x4502}, "Power Blue Splat"},
    {{3406, 0x0000}, "Stormblade"},
    {{3411, 0x0000}, "Smash Hit"},
    {{3411, 0x4502}, "Steel Plated Smash Hit"},
    {{3412, 0x0000}, "Spitfire"},
    {{3412, 0x4502}, "Dark Spitfire"},
    {{3413, 0x0000}, "Hurricane Jet Vac"},
    {{3413, 0x4503}, "Legendary Hurricane Jet Vac"},
    {{3414, 0x0000}, "Double Dare Trigger Happy"},
    {{3414, 0x4502}, "Power Blue Double Dare Trigger Happy"},
    {{3415, 0x0000}, "Super Shot Stealth Elf"},
    {{3415, 0x4502}, "Dark Super Shot Stealth Elf"},
    {{3416, 0x0000}, "Shark Shooter Terrafin"},
    {{3417, 0x0000}, "Bone Bash Roller Brawl"},
    {{3417, 0x4503}, "Legendary Bone Bash Roller Brawl"},
    {{3420, 0x0000}, "Big Bubble Pop Fizz"},
    {{3420, 0x450E}, "Birthday Bash Big Bubble Pop Fizz"},
    {{3421, 0x0000}, "Lava Lance Eruptor"},
    {{3422, 0x0000}, "Deep Dive Gill Grunt"},
    //{{3423, 0x0000}, "Turbo Charge Donkey Kong (Nintendo Only)"},
    //{{3423, 0x4502}, "Dark Turbo Charge Donkey Kong (Nintendo Only)"},
    //{{3424, 0x0000}, "Hammer Slam Bowser (Nintendo Only)"},
    //{{3424, 0x4502}, "Dark Hammer Slam Bowser (Nintendo Only)"},
    {{3425, 0x0000}, "Dive-Clops"},
    {{3425, 0x450E}, "Missile-Tow Dive-Clops"},
    {{3426, 0x0000}, "Astroblast"},
    {{3426, 0x4503}, "Legendary Astroblast"},
    {{3427, 0x0000}, "Nightfall"},
    {{3428, 0x0000}, "Thrillipede"},
    {{3428, 0x450D}, "Eggcited Thrillipede"},
    {{3500, 0x0000}, "Sky Trophy"},
    {{3501, 0x0000}, "Land Trophy"},
    {{3502, 0x0000}, "Sea Trophy"},
    {{3503, 0x0000}, "Kaos Trophy"},
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
