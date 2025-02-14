#include "stdafx.h"
#include "Utilities/File.h"
#include "Crypto/aes.h"
#include "Crypto/sha1.h"
#include "infinity_dialog.h"
#include "Emu/Io/Infinity.h"

#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QComboBox>
#include <QPushButton>
#include <QStringList>
#include <QCompleter>

infinity_dialog* infinity_dialog::inst = nullptr;
std::array<std::optional<u32>, 9> infinity_dialog::figure_slots = {};
static QString s_last_figure_path;

LOG_CHANNEL(infinity_log, "infinity");

static constexpr std::array<u8, 16> BLANK_BLOCK = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr std::array<u8, 32> SHA1_CONSTANT = {
	0xAF, 0x62, 0xD2, 0xEC, 0x04, 0x91, 0x96, 0x8C, 0xC5, 0x2A, 0x1A, 0x71, 0x65, 0xF8, 0x65, 0xFE,
	0x28, 0x63, 0x29, 0x20, 0x44, 0x69, 0x73, 0x6e, 0x65, 0x79, 0x20, 0x32, 0x30, 0x31, 0x33};

const std::map<const u32, const std::pair<const u8, const std::string>> list_figures = {
	{0x0F4241, {1, "Mr. Incredible"}},
	{0x0F4242, {1, "Sulley"}},
	{0x0F4243, {1, "Jack Sparrow"}},
	{0x0F4244, {1, "Lone Ranger"}},
	{0x0F4245, {1, "Tonto"}},
	{0x0F4246, {1, "Lightning McQueen"}},
	{0x0F4247, {1, "Holley Shiftwell"}},
	{0x0F4248, {1, "Buzz Lightyear"}},
	{0x0F4249, {1, "Jessie"}},
	{0x0F424A, {1, "Mike"}},
	{0x0F424B, {1, "Mrs. Incredible"}},
	{0x0F424C, {1, "Hector Barbossa"}},
	{0x0F424D, {1, "Davy Jones"}},
	{0x0F424E, {1, "Randy"}},
	{0x0F424F, {1, "Syndrome"}},
	{0x0F4250, {1, "Woody"}},
	{0x0F4251, {1, "Mater"}},
	{0x0F4252, {1, "Dash"}},
	{0x0F4253, {1, "Violet"}},
	{0x0F4254, {1, "Francesco Bernoulli"}},
	{0x0F4255, {1, "Sorcerer's Apprentice Mickey"}},
	{0x0F4256, {1, "Jack Skellington"}},
	{0x0F4257, {1, "Rapunzel"}},
	{0x0F4258, {1, "Anna"}},
	{0x0F4259, {1, "Elsa"}},
	{0x0F425A, {1, "Phineas"}},
	{0x0F425B, {1, "Agent P"}},
	{0x0F425C, {1, "Wreck-It Ralph"}},
	{0x0F425D, {1, "Vanellope"}},
	{0x0F425E, {1, "Mr. Incredible (Crystal)"}},
	{0x0F425F, {1, "Jack Sparrow (Crystal)"}},
	{0x0F4260, {1, "Sulley (Crystal)"}},
	{0x0F4261, {1, "Lightning McQueen (Crystal)"}},
	{0x0F4262, {1, "Lone Ranger (Crystal)"}},
	{0x0F4263, {1, "Buzz Lightyear (Crystal)"}},
	{0x0F4264, {1, "Agent P (Crystal)"}},
	{0x0F4265, {1, "Sorcerer's Apprentice Mickey (Crystal)"}},
	{0x0F4266, {1, "Buzz Lightyear (Glowing)"}},
	{0x0F42A4, {2, "Captain America"}},
	{0x0F42A5, {2, "Hulk"}},
	{0x0F42A6, {2, "Iron Man"}},
	{0x0F42A7, {2, "Thor"}},
	{0x0F42A8, {2, "Groot"}},
	{0x0F42A9, {2, "Rocket Raccoon"}},
	{0x0F42AA, {2, "Star-Lord"}},
	{0x0F42AB, {2, "Spider-Man"}},
	{0x0F42AC, {2, "Nick Fury"}},
	{0x0F42AD, {2, "Black Widow"}},
	{0x0F42AE, {2, "Hawkeye"}},
	{0x0F42AF, {2, "Drax"}},
	{0x0F42B0, {2, "Gamora"}},
	{0x0F42B1, {2, "Iron Fist"}},
	{0x0F42B2, {2, "Nova"}},
	{0x0F42B3, {2, "Venom"}},
	{0x0F42B4, {2, "Donald Duck"}},
	{0x0F42B5, {2, "Aladdin"}},
	{0x0F42B6, {2, "Stitch"}},
	{0x0F42B7, {2, "Merida"}},
	{0x0F42B8, {2, "Tinker Bell"}},
	{0x0F42B9, {2, "Maleficent"}},
	{0x0F42BA, {2, "Hiro"}},
	{0x0F42BB, {2, "Baymax"}},
	{0x0F42BC, {2, "Loki"}},
	{0x0F42BD, {2, "Ronan"}},
	{0x0F42BE, {2, "Green Goblin"}},
	{0x0F42BF, {2, "Falcon"}},
	{0x0F42C0, {2, "Yondu"}},
	{0x0F42C1, {2, "Jasmine"}},
	{0x0F42C6, {2, "Black Suit Spider-Man"}},
	{0x0F42D6, {3, "Sam Flynn"}},
	{0x0F42D7, {3, "Quorra"}},
	{0x0F4308, {3, "Anakin Skywalker"}},
	{0x0F4309, {3, "Obi-Wan Kenobi"}},
	{0x0F430A, {3, "Yoda"}},
	{0x0F430B, {3, "Ahsoka Tano"}},
	{0x0F430C, {3, "Darth Maul"}},
	{0x0F430E, {3, "Luke Skywalker"}},
	{0x0F430F, {3, "Han Solo"}},
	{0x0F4310, {3, "Princess Leia"}},
	{0x0F4311, {3, "Chewbacca"}},
	{0x0F4312, {3, "Darth Vader"}},
	{0x0F4313, {3, "Boba Fett"}},
	{0x0F4314, {3, "Ezra Bridger"}},
	{0x0F4315, {3, "Kanan Jarrus"}},
	{0x0F4316, {3, "Sabine Wren"}},
	{0x0F4317, {3, "Zeb Orrelios"}},
	{0x0F4318, {3, "Joy"}},
	{0x0F4319, {3, "Anger"}},
	{0x0F431A, {3, "Fear"}},
	{0x0F431B, {3, "Sadness"}},
	{0x0F431C, {3, "Disgust"}},
	{0x0F431D, {3, "Mickey Mouse"}},
	{0x0F431E, {3, "Minnie Mouse"}},
	{0x0F431F, {3, "Mulan"}},
	{0x0F4320, {3, "Olaf"}},
	{0x0F4321, {3, "Vision"}},
	{0x0F4322, {3, "Ultron"}},
	{0x0F4323, {3, "Ant-Man"}},
	{0x0F4325, {3, "Captain America - The First Avenger"}},
	{0x0F4326, {3, "Finn"}},
	{0x0F4327, {3, "Kylo Ren"}},
	{0x0F4328, {3, "Poe Dameron"}},
	{0x0F4329, {3, "Rey"}},
	{0x0F432B, {3, "Spot"}},
	{0x0F432C, {3, "Nick Wilde"}},
	{0x0F432D, {3, "Judy Hopps"}},
	{0x0F432E, {3, "Hulkbuster"}},
	{0x0F432F, {3, "Anakin Skywalker (Light FX)"}},
	{0x0F4330, {3, "Obi-Wan Kenobi (Light FX)"}},
	{0x0F4331, {3, "Yoda (Light FX)"}},
	{0x0F4332, {3, "Luke Skywalker (Light FX)"}},
	{0x0F4333, {3, "Darth Vader (Light FX)"}},
	{0x0F4334, {3, "Kanan Jarrus (Light FX)"}},
	{0x0F4335, {3, "Kylo Ren (Light FX)"}},
	{0x0F4336, {3, "Black Panther"}},
	{0x0F436C, {3, "Nemo"}},
	{0x0F436D, {3, "Dory"}},
	{0x0F436E, {3, "Baloo"}},
	{0x0F436F, {3, "Alice"}},
	{0x0F4370, {3, "Mad Hatter"}},
	{0x0F4371, {3, "Time"}},
	{0x0F4372, {3, "Peter Pan"}},
	{0x1E8481, {1, "Starter Play Set"}},
	{0x1E8482, {1, "Lone Ranger Play Set"}},
	{0x1E8483, {1, "Cars Play Set"}},
	{0x1E8484, {1, "Toy Story in Space Play Set"}},
	{0x1E84E4, {2, "Marvel's The Avengers Play Set"}},
	{0x1E84E5, {2, "Marvel's Spider-Man Play Set"}},
	{0x1E84E6, {2, "Marvel's Guardians of the Galaxy Play Set"}},
	{0x1E84E7, {2, "Assault on Asgard"}},
	{0x1E84E8, {2, "Escape from the Kyln"}},
	{0x1E84E9, {2, "Stitch's Tropical Rescue"}},
	{0x1E84EA, {2, "Brave Forest Siege"}},
	{0x1E8548, {3, "Inside Out Play Set"}},
	{0x1E8549, {3, "Star Wars: Twilight of the Republic Play Set"}},
	{0x1E854A, {3, "Star Wars: Rise Against the Empire Play Set"}},
	{0x1E854B, {3, "Star Wars: The Force Awakens Play Set"}},
	{0x1E854C, {3, "Marvel Battlegrounds Play Set"}},
	{0x1E854D, {3, "Toy Box Speedway"}},
	{0x1E854E, {3, "Toy Box Takeover"}},
	{0x1E85AC, {3, "Finding Dory Play Set"}},
	{0x2DC6C3, {1, "Bolt's Super Strength"}},
	{0x2DC6C4, {1, "Ralph's Power of Destruction"}},
	{0x2DC6C5, {1, "Chernabog's Power"}},
	{0x2DC6C6, {1, "C.H.R.O.M.E. Damage Increaser"}},
	{0x2DC6C7, {1, "Dr. Doofenshmirtz's Damage-Inator!"}},
	{0x2DC6C8, {1, "Electro-Charge"}},
	{0x2DC6C9, {1, "Fix-It Felix's Repair Power"}},
	{0x2DC6CA, {1, "Rapunzel's Healing"}},
	{0x2DC6CB, {1, "C.H.R.O.M.E. Armor Shield"}},
	{0x2DC6CC, {1, "Star Command Shield"}},
	{0x2DC6CD, {1, "Violet's Force Field"}},
	{0x2DC6CE, {1, "Pieces of Eight"}},
	{0x2DC6CF, {1, "Scrooge McDuck's Lucky Dime"}},
	{0x2DC6D0, {1, "User Control"}},
	{0x2DC6D1, {1, "Sorcerer Mickey's Hat"}},
	{0x2DC6FE, {1, "Emperor Zurg's Wrath"}},
	{0x2DC6FF, {1, "Merlin's Summon"}},
	{0x2DC765, {2, "Enchanted Rose"}},
	{0x2DC766, {2, "Mulan's Training Uniform"}},
	{0x2DC767, {2, "Flubber"}},
	{0x2DC768, {2, "S.H.I.E.L.D. Helicarrier Strike"}},
	{0x2DC769, {2, "Zeus' Thunderbolts"}},
	{0x2DC76A, {2, "King Louie's Monkeys"}},
	{0x2DC76B, {2, "Infinity Gauntlet"}},
	{0x2DC76D, {2, "Sorcerer Supreme"}},
	{0x2DC76E, {2, "Maleficent's Spell Cast"}},
	{0x2DC76F, {2, "Chernabog's Spirit Cyclone"}},
	{0x2DC770, {2, "Marvel Team-Up: Capt. Marvel"}},
	{0x2DC771, {2, "Marvel Team-Up: Iron Patriot"}},
	{0x2DC772, {2, "Marvel Team-Up: Ant-Man"}},
	{0x2DC773, {2, "Marvel Team-Up: White Tiger"}},
	{0x2DC774, {2, "Marvel Team-Up: Yondu"}},
	{0x2DC775, {2, "Marvel Team-Up: Winter Soldier"}},
	{0x2DC776, {2, "Stark Arc Reactor"}},
	{0x2DC777, {2, "Gamma Rays"}},
	{0x2DC778, {2, "Alien Symbiote"}},
	{0x2DC779, {2, "All for One"}},
	{0x2DC77A, {2, "Sandy Claws Surprise"}},
	{0x2DC77B, {2, "Glory Days"}},
	{0x2DC77C, {2, "Cursed Pirate Gold"}},
	{0x2DC77D, {2, "Sentinel of Liberty"}},
	{0x2DC77E, {2, "The Immortal Iron Fist"}},
	{0x2DC77F, {2, "Space Armor"}},
	{0x2DC780, {2, "Rags to Riches"}},
	{0x2DC781, {2, "Ultimate Falcon"}},
	{0x2DC788, {3, "Tomorrowland Time Bomb"}},
	{0x2DC78E, {3, "Galactic Team-Up: Mace Windu"}},
	{0x2DC791, {3, "Luke's Rebel Alliance Flight Suit Costume"}},
	{0x2DC798, {3, "Finn's Stormtrooper Costume"}},
	{0x2DC799, {3, "Poe's Resistance Jacket"}},
	{0x2DC79A, {3, "Resistance Tactical Strike"}},
	{0x2DC79E, {3, "Officer Nick Wilde"}},
	{0x2DC79F, {3, "Meter Maid Judy"}},
	{0x2DC7A2, {3, "Darkhawk's Blast"}},
	{0x2DC7A3, {3, "Cosmic Cube Blast"}},
	{0x2DC7A4, {3, "Princess Leia's Boushh Disguise"}},
	{0x2DC7A6, {3, "Nova Corps Strike"}},
	{0x2DC7A7, {3, "King Mickey"}},
	{0x3D0912, {1, "Mickey's Car"}},
	{0x3D0913, {1, "Cinderella's Coach"}},
	{0x3D0914, {1, "Electric Mayhem Bus"}},
	{0x3D0915, {1, "Cruella De Vil's Car"}},
	{0x3D0916, {1, "Pizza Planet Delivery Truck"}},
	{0x3D0917, {1, "Mike's New Car"}},
	{0x3D0919, {1, "Parking Lot Tram"}},
	{0x3D091A, {1, "Captain Hook's Ship"}},
	{0x3D091B, {1, "Dumbo"}},
	{0x3D091C, {1, "Calico Helicopter"}},
	{0x3D091D, {1, "Maximus"}},
	{0x3D091E, {1, "Angus"}},
	{0x3D091F, {1, "Abu the Elephant"}},
	{0x3D0920, {1, "Headless Horseman's Horse"}},
	{0x3D0921, {1, "Phillipe"}},
	{0x3D0922, {1, "Khan"}},
	{0x3D0923, {1, "Tantor"}},
	{0x3D0924, {1, "Dragon Firework Cannon"}},
	{0x3D0925, {1, "Stitch's Blaster"}},
	{0x3D0926, {1, "Toy Story Mania Blaster"}},
	{0x3D0927, {1, "Flamingo Croquet Mallet"}},
	{0x3D0928, {1, "Carl Fredricksen's Cane"}},
	{0x3D0929, {1, "Hangin' Ten Stitch With Surfboard"}},
	{0x3D092A, {1, "Condorman Glider"}},
	{0x3D092B, {1, "WALL-E's Fire Extinguisher"}},
	{0x3D092C, {1, "On the Grid"}},
	{0x3D092D, {1, "WALL-E's Collection"}},
	{0x3D092E, {1, "King Candy's Dessert Toppings"}},
	{0x3D0930, {1, "Victor's Experiments"}},
	{0x3D0931, {1, "Jack's Scary Decorations"}},
	{0x3D0933, {1, "Frozen Flourish"}},
	{0x3D0934, {1, "Rapunzel's Kingdom"}},
	{0x3D0935, {1, "TRON Interface"}},
	{0x3D0936, {1, "Buy N Large Atmosphere"}},
	{0x3D0937, {1, "Sugar Rush Sky"}},
	{0x3D0939, {1, "New Holland Skyline"}},
	{0x3D093A, {1, "Halloween Town Sky"}},
	{0x3D093C, {1, "Chill in the Air"}},
	{0x3D093D, {1, "Rapunzel's Birthday Sky"}},
	{0x3D0940, {1, "Astro Blasters Space Cruiser"}},
	{0x3D0941, {1, "Marlin's Reef"}},
	{0x3D0942, {1, "Nemo's Seascape"}},
	{0x3D0943, {1, "Alice's Wonderland"}},
	{0x3D0944, {1, "Tulgey Wood"}},
	{0x3D0945, {1, "Tri-State Area Terrain"}},
	{0x3D0946, {1, "Danville Sky"}},
	{0x3D0965, {2, "Stark Tech"}},
	{0x3D0966, {2, "Spider-Streets"}},
	{0x3D0967, {2, "World War Hulk"}},
	{0x3D0968, {2, "Gravity Falls Forest"}},
	{0x3D0969, {2, "Neverland"}},
	{0x3D096A, {2, "Simba's Pridelands"}},
	{0x3D096C, {2, "Calhoun's Command"}},
	{0x3D096D, {2, "Star-Lord's Galaxy"}},
	{0x3D096E, {2, "Dinosaur World"}},
	{0x3D096F, {2, "Groot's Roots"}},
	{0x3D0970, {2, "Mulan's Countryside"}},
	{0x3D0971, {2, "The Sands of Agrabah"}},
	{0x3D0974, {2, "A Small World"}},
	{0x3D0975, {2, "View from the Suit"}},
	{0x3D0976, {2, "Spider-Sky"}},
	{0x3D0977, {2, "World War Hulk Sky"}},
	{0x3D0978, {2, "Gravity Falls Sky"}},
	{0x3D0979, {2, "Second Star to the Right"}},
	{0x3D097A, {2, "The King's Domain"}},
	{0x3D097C, {2, "CyBug Swarm"}},
	{0x3D097D, {2, "The Rip"}},
	{0x3D097E, {2, "Forgotten Skies"}},
	{0x3D097F, {2, "Groot's View"}},
	{0x3D0980, {2, "The Middle Kingdom"}},
	{0x3D0984, {2, "Skies of the World"}},
	{0x3D0985, {2, "S.H.I.E.L.D. Containment Truck"}},
	{0x3D0986, {2, "Main Street Electrical Parade Float"}},
	{0x3D0987, {2, "Mr. Toad's Motorcar"}},
	{0x3D0988, {2, "Le Maximum"}},
	{0x3D0989, {2, "Alice in Wonderland's Caterpillar"}},
	{0x3D098A, {2, "Eglantine's Motorcycle"}},
	{0x3D098B, {2, "Medusa's Swamp Mobile"}},
	{0x3D098C, {2, "Hydra Motorcycle"}},
	{0x3D098D, {2, "Darkwing Duck's Ratcatcher"}},
	{0x3D098F, {2, "The USS Swinetrek"}},
	{0x3D0991, {2, "Spider-Copter"}},
	{0x3D0992, {2, "Aerial Area Rug"}},
	{0x3D0993, {2, "Jack-O-Lantern's Glider"}},
	{0x3D0994, {2, "Spider-Buggy"}},
	{0x3D0995, {2, "Jack Skellington's Reindeer"}},
	{0x3D0996, {2, "Fantasyland Carousel Horse"}},
	{0x3D0997, {2, "Odin's Horse"}},
	{0x3D0998, {2, "Gus the Mule"}},
	{0x3D099A, {2, "Darkwing Duck's Grappling Gun"}},
	{0x3D099C, {2, "Ghost Rider's Chain Whip"}},
	{0x3D099D, {2, "Lew Zealand's Boomerang Fish"}},
	{0x3D099E, {2, "Sergeant Calhoun's Blaster"}},
	{0x3D09A0, {2, "Falcon's Wings"}},
	{0x3D09A1, {2, "Mabel's Kittens for Fists"}},
	{0x3D09A2, {2, "Jim Hawkins' Solar Board"}},
	{0x3D09A3, {2, "Black Panther's Vibranium Knives"}},
	{0x3D09A4, {2, "Cloak of Levitation"}},
	{0x3D09A5, {2, "Aladdin's Magic Carpet"}},
	{0x3D09A6, {2, "Honey Lemon's Ice Capsules"}},
	{0x3D09A7, {2, "Jasmine's Palace View"}},
	{0x3D09C1, {2, "Lola"}},
	{0x3D09C2, {2, "Spider-Cycle"}},
	{0x3D09C3, {2, "The Avenjet"}},
	{0x3D09C4, {2, "Spider-Glider"}},
	{0x3D09C5, {2, "Light Cycle"}},
	{0x3D09C6, {2, "Light Jet"}},
	{0x3D09C9, {3, "Retro Ray Gun"}},
	{0x3D09CA, {3, "Tomorrowland Futurescape"}},
	{0x3D09CB, {3, "Tomorrowland Stratosphere"}},
	{0x3D09CC, {3, "Skies Over Felucia"}},
	{0x3D09CD, {3, "Forests of Felucia"}},
	{0x3D09CF, {3, "General Grievous' Wheel Bike"}},
	{0x3D09D2, {3, "Slave I Flyer"}},
	{0x3D09D3, {3, "Y-Wing Fighter"}},
	{0x3D09D4, {3, "Arlo"}},
	{0x3D09D5, {3, "Nash"}},
	{0x3D09D6, {3, "Butch"}},
	{0x3D09D7, {3, "Ramsey"}},
	{0x3D09DC, {3, "Stars Over Sahara Square"}},
	{0x3D09DD, {3, "Sahara Square Sands"}},
	{0x3D09E0, {3, "Ghost Rider's Motorcycle"}},
	{0x3D09E5, {3, "Quad Jumper"}}};

u32 infinity_crc32(u16 init_value, const u8* buffer, u32 size)
{
	const std::array<u32, 256> CRC32_TABLE = {
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
	u32 crc = init_value;

	for (u32 i = 0; i < size; i++)
	{
		u8 index = u8(crc & 0xFF) ^ buffer[i];
		crc = ((crc >> 8) ^ CRC32_TABLE[index]);
	}

	return crc;
}

figure_creator_dialog::figure_creator_dialog(QWidget* parent, u8 slot)
	: QDialog(parent)
{
	setWindowTitle(tr("Figure Creator"));
	setObjectName("figure_creator");
	setMinimumSize(QSize(500, 150));

	QVBoxLayout* vbox_panel = new QVBoxLayout();

	QComboBox* combo_figlist = new QComboBox();
	QStringList filterlist;
	u32 first_entry = 0;

	for (const auto& [figure, entry] : list_figures)
	{
		// Only display entry if it is a piece appropriate for the slot
		if ((slot == 0 &&
				((figure > 0x1E8480 && figure < 0x2DC6BF) || (figure > 0x3D0900 && figure < 0x4C4B3F))) ||
			((slot == 1 || slot == 2) && (figure > 0x3D0900 && figure < 0x4C4B3F)) ||
			((slot == 3 || slot == 6) && figure < 0x1E847F) ||
			((slot == 4 || slot == 5 || slot == 7 || slot == 8) &&
				(figure > 0x2DC6C0 && figure < 0x3D08FF)))
		{
			const auto& [num, figure_name] = entry;
			const u32 qnum = (figure << 8) | num;
			QString name = QString::fromStdString(figure_name);
			combo_figlist->addItem(name, QVariant(qnum));
			filterlist << std::move(name);
			if (first_entry == 0)
			{
				first_entry = figure;
			}
		}
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

	QHBoxLayout* hbox_number = new QHBoxLayout();
	QLabel* label_number = new QLabel(tr("Figure Number:"));
	QLineEdit* edit_number = new QLineEdit(QString::number(first_entry));
	QLabel* label_series = new QLabel(tr("Series:"));
	QLineEdit* edit_series = new QLineEdit("1");
	QRegularExpressionValidator* rxv = new QRegularExpressionValidator(QRegularExpression("\\d*"), this);
	QIntValidator* valid_series = new QIntValidator(1, 3, this);
	edit_number->setValidator(rxv);
	edit_series->setValidator(valid_series);
	hbox_number->addWidget(label_number);
	hbox_number->addWidget(edit_number);
	hbox_number->addWidget(label_series);
	hbox_number->addWidget(edit_series);
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
			const u32 fig_info = combo_figlist->itemData(index).toUInt();
			if (fig_info != 0xFFFFFFFF)
			{
				const u32 fig_num = fig_info >> 8;
				const u8 series = fig_info & 0xFF;

				edit_number->setText(QString::number(fig_num));
				edit_series->setText(QString::number(series));
			}
		});

	connect(btn_create, &QAbstractButton::clicked, this, [=, this]()
		{
			bool ok_num = false, ok_series = false;
			const u32 fig_num = edit_number->text().toULong(&ok_num);
			if (!ok_num)
			{
				QMessageBox::warning(this, tr("Error converting value"), tr("Figure number entered is invalid!"), QMessageBox::Ok);
				return;
			}
			const u8 series = edit_series->text().toUShort(&ok_series);
			if (!ok_series || series > 3 || series < 1)
			{
				QMessageBox::warning(this, tr("Error converting value"), tr("Series number entered is invalid!"), QMessageBox::Ok);
				return;
			}
			const auto found_figure = list_figures.find(fig_num);
			if (found_figure != list_figures.cend())
			{
				s_last_figure_path += QString::fromStdString(found_figure->second.second + ".bin");
			}
			else
			{
				s_last_figure_path += QString("Unknown(%1 %2).bin").arg(fig_num).arg(series);
			}

			m_file_path = QFileDialog::getSaveFileName(this, tr("Create Figure File"), s_last_figure_path, tr("Infinity Figure (*.bin);;"));
			if (m_file_path.isEmpty())
			{
				return;
			}
			if (!create_blank_figure(fig_num, series))
			{
				QMessageBox::warning(this, tr("Failed to create figure file!"), tr("Failed to create figure file:\n%1").arg(m_file_path), QMessageBox::Ok);
				return;
			}

			s_last_figure_path = QFileInfo(m_file_path).absolutePath() + "/";
			accept();
		});

	connect(btn_cancel, &QAbstractButton::clicked, this, &QDialog::reject);

	connect(co_compl, QOverload<const QString&>::of(&QCompleter::activated), [=](const QString& text)
		{
			combo_figlist->setCurrentIndex(combo_figlist->findText(text));
		});
}

bool figure_creator_dialog::create_blank_figure(u32 character, u8 series)
{
	infinity_log.trace("File path: %s Character: %d Series: %d", m_file_path, character, series);
	fs::file inf_file(m_file_path.toStdString(), fs::read + fs::write + fs::create);
	if (!inf_file)
	{
		return false;
	}
	// Create a 320 byte file with standard NFC read/write permissions
	std::array<u8, 0x14 * 0x10> file_data{};
	u32 first_block = 0x17878E;
	u32 other_blocks = 0x778788;
	for (u8 i = 0; i < 3; i++)
	{
		file_data[0x36 + i] = u8((first_block >> (2 - i) * 8) & 0xFF);
	}
	for (u32 index = 1; index < 5; index++)
	{
		for (u8 i = 0; i < 3; i++)
		{
			file_data[((index * 0x40) + 0x36) + i] = u8((other_blocks >> (2 - i) * 8) & 0xFF);
		}
	}
	// Create the vector to calculate the SHA1 hash with
	std::vector<u8> sha1_calc = {SHA1_CONSTANT.begin(), SHA1_CONSTANT.end() - 1};
	// Generate random UID, used for AES encrypt/decrypt
	std::array<u8, 16> uid_data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 0x44, 0x00, 0xC2};
	for (u8 i = 0; i < 7; i++)
	{
		u8 random = rand() % 255;
		sha1_calc.push_back(random);
		uid_data[i] = random;
	}

	std::array<u8, 16> figure_data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, series, 0xD1, 0x1F};

	// Figure Number, input by end user
	figure_data[1] = u8((character >> 16) & 0xFF);
	figure_data[2] = u8((character >> 8) & 0xFF);
	figure_data[3] = u8(character & 0xFF);

	// Manufacture date, formatted as YY/MM/DD. Set to release date of figure's series
	if (series == 1)
	{
		figure_data[4] = 0x0D;
		figure_data[5] = 0x08;
		figure_data[6] = 0x12;
	}
	else if (series == 2)
	{
		figure_data[4] = 0x0E;
		figure_data[5] = 0x09;
		figure_data[6] = 0x12;
	}
	else if (series == 3)
	{
		figure_data[4] = 0x0F;
		figure_data[5] = 0x08;
		figure_data[6] = 0x1C;
	}

	u32 checksum = infinity_crc32(0, figure_data.data(), 12);
	for (s8 i = 0; i < 4; i++)
	{
		figure_data[12 + i] = u8((checksum >> (3 - i) * 8) & 0xFF);
	}

	if (figure_data[1] == 0)
		return false;

	sha1_context ctx;
	u8 output[20];

	sha1_starts(&ctx);
	sha1_update(&ctx, sha1_calc.data(), sha1_calc.size());
	sha1_finish(&ctx, output);

	u8 key[0x10];
	for (int i = 0; i < 4; i++)
	{
		for (int x = 0; x < 4; x++)
		{
			key[x + (i * 4)] = output[(3 - x) + (i * 4)];
		}
	}

	// Create AES Encrypt context based on AES key, use this to encrypt the character data and 4 blank
	// blocks
	aes_context aes;
	aes_setkey_enc(&aes, key, 128);
	std::array<u8, 16> encrypted_block{};
	std::array<u8, 16> encrypted_blank{};
	aes_crypt_ecb(&aes, AES_ENCRYPT, figure_data.data(), encrypted_block.data());
	aes_crypt_ecb(&aes, AES_ENCRYPT, BLANK_BLOCK.data(), encrypted_blank.data());

	// Copy encrypted data and UID data to the Figure File
	memcpy(&file_data[0], uid_data.data(), uid_data.size());
	memcpy(&file_data[16], encrypted_block.data(), encrypted_block.size());
	memcpy(&file_data[16 * 0x04], encrypted_blank.data(), encrypted_blank.size());
	memcpy(&file_data[16 * 0x08], encrypted_blank.data(), encrypted_blank.size());
	memcpy(&file_data[16 * 0x0C], encrypted_blank.data(), encrypted_blank.size());
	memcpy(&file_data[16 * 0x0D], encrypted_blank.data(), encrypted_blank.size());

	inf_file.write(file_data.data(), file_data.size());
	inf_file.close();

	return true;
}

QString figure_creator_dialog::get_file_path() const
{
	return m_file_path;
}

infinity_dialog::infinity_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Infinity Manager"));
	setObjectName("infinity_manager");
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

	QGroupBox* group_figures = new QGroupBox(tr("Active Infinity Figures:"));
	QVBoxLayout* vbox_group = new QVBoxLayout();

	add_figure_slot(vbox_group, QString(tr("Play Set/Power Disc")), 0);
	add_line(vbox_group);
	add_figure_slot(vbox_group, QString(tr("Power Disc Two")), 1);
	add_line(vbox_group);
	add_figure_slot(vbox_group, QString(tr("Power Disc Three")), 2);
	add_line(vbox_group);
	add_figure_slot(vbox_group, QString(tr("Player One")), 3);
	add_line(vbox_group);
	add_figure_slot(vbox_group, QString(tr("Player One Ability One")), 4);
	add_line(vbox_group);
	add_figure_slot(vbox_group, QString(tr("Player One Ability Two")), 5);
	add_line(vbox_group);
	add_figure_slot(vbox_group, QString(tr("Player Two")), 6);
	add_line(vbox_group);
	add_figure_slot(vbox_group, QString(tr("Player Two Ability One")), 7);
	add_line(vbox_group);
	add_figure_slot(vbox_group, QString(tr("Player Two Ability Two")), 8);

	group_figures->setLayout(vbox_group);
	vbox_panel->addWidget(group_figures);
	setLayout(vbox_panel);
}

infinity_dialog::~infinity_dialog()
{
	inst = nullptr;
}

infinity_dialog* infinity_dialog::get_dlg(QWidget* parent)
{
	if (inst == nullptr)
		inst = new infinity_dialog(parent);

	return inst;
}

void infinity_dialog::add_figure_slot(QVBoxLayout* vbox_group, QString name, u8 slot)
{
	ensure(slot < figure_slots.size());

	QHBoxLayout* hbox_infinity = new QHBoxLayout();

	QLabel* label_figname = new QLabel(name);

	QPushButton* clear_btn = new QPushButton(tr("Clear"));
	QPushButton* create_btn = new QPushButton(tr("Create"));
	QPushButton* load_btn = new QPushButton(tr("Load"));

	m_edit_figures[slot] = new QLineEdit();
	m_edit_figures[slot]->setEnabled(false);
	if (figure_slots[slot])
	{
		const auto found_figure = list_figures.find(figure_slots[slot].value());
		if (found_figure != list_figures.cend())
		{
			m_edit_figures[slot]->setText(QString::fromStdString(found_figure->second.second));
		}
		else
		{
			m_edit_figures[slot]->setText(tr("Unknown Figure"));
		}
	}
	else
	{
		m_edit_figures[slot]->setText(tr("None"));
	}

	connect(clear_btn, &QAbstractButton::clicked, this, [this, slot]
		{
			clear_figure(slot);
		});
	connect(create_btn, &QAbstractButton::clicked, this, [this, slot]
		{
			create_figure(slot);
		});
	connect(load_btn, &QAbstractButton::clicked, this, [this, slot]
		{
			load_figure(slot);
		});

	hbox_infinity->addWidget(label_figname);
	hbox_infinity->addWidget(m_edit_figures[slot]);
	hbox_infinity->addWidget(clear_btn);
	hbox_infinity->addWidget(create_btn);
	hbox_infinity->addWidget(load_btn);

	vbox_group->addLayout(hbox_infinity);
}

void infinity_dialog::clear_figure(u8 slot)
{
	ensure(slot < figure_slots.size());

	if (figure_slots[slot])
	{
		g_infinitybase.remove_figure(slot);
		figure_slots[slot] = 0;
		m_edit_figures[slot]->setText(tr("None"));
	}
}

void infinity_dialog::create_figure(u8 slot)
{
	ensure(slot < figure_slots.size());
	figure_creator_dialog create_dlg(this, slot);
	if (create_dlg.exec() == Accepted)
	{
		load_figure_path(slot, create_dlg.get_file_path());
	}
}

void infinity_dialog::load_figure(u8 slot)
{
	ensure(slot < figure_slots.size());
	const QString file_path = QFileDialog::getOpenFileName(this, tr("Select Infinity File"), s_last_figure_path, tr("Infinity Figure (*.bin);;"));
	if (file_path.isEmpty())
	{
		return;
	}

	s_last_figure_path = QFileInfo(file_path).absolutePath() + "/";

	load_figure_path(slot, file_path);
}

void infinity_dialog::load_figure_path(u8 slot, const QString& path)
{
	fs::file inf_file(path.toStdString(), fs::read + fs::write + fs::lock);
	if (!inf_file)
	{
		QMessageBox::warning(this, tr("Failed to open the figure file!"), tr("Failed to open the figure file(%1)!\nFile may already be in use on the base.").arg(path), QMessageBox::Ok);
		return;
	}

	std::array<u8, 0x14 * 0x10> data;
	if (inf_file.read(data.data(), data.size()) != data.size())
	{
		QMessageBox::warning(this, tr("Failed to read the figure file!"), tr("Failed to read the figure file(%1)!\nFile was too small.").arg(path), QMessageBox::Ok);
		return;
	}

	clear_figure(slot);

	const u32 fignum = g_infinitybase.load_figure(data, std::move(inf_file), slot);
	const auto name = list_figures.find(fignum);
	if (name != list_figures.cend())
	{
		m_edit_figures[slot]->setText(QString::fromStdString(name->second.second));
	}
	else
	{
		m_edit_figures[slot]->setText(tr("Unknown Figure"));
	}
	figure_slots[slot] = fignum;
}
