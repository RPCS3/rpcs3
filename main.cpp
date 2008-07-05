#include "parser.h"
#include "tests.h"
#include <fstream>
#include <iostream>

struct Vec3 {
	float x, y, z;

	friend std::ostream& operator << (std::ostream& out, const Vec3& v) {
		out << v.x << " " << v.y << " " << v.z;
		return out;
	}
};

void operator >> (const YAML::Node& node, Vec3& v)
{
	node[0] >> v.x;
	node[1] >> v.y;
	node[2] >> v.z;
}

struct Room {
	std::string name;
	Vec3 pos, size;
	float height;

	friend std::ostream& operator << (std::ostream& out, const Room& room) {
		out << "Name: " << room.name << std::endl;
		out << "Pos: " << room.pos << std::endl;
		out << "Size: " << room.size << std::endl;
		out << "Height: " << room.height << std::endl;
		return out;
	}
};

void operator >> (const YAML::Node& node, Room& room)
{
	node["name"] >> room.name;
	node["pos"] >> room.pos;
	node["size"] >> room.size;
	node["height"] >> room.height;
}

struct Level {
	std::vector <Room> rooms;

	friend std::ostream& operator << (std::ostream& out, const Level& level) {
		for(unsigned i=0;i<level.rooms.size();i++) {
			out << level.rooms[i];
			out << "---------------------------------------\n";
		}
		return out;
	}
};

void operator >> (const YAML::Node& node, Level& level)
{
	const YAML::Node& rooms = node["rooms"];
	for(unsigned i=0;i<rooms.size();i++) {
		Room room;
		rooms[i] >> room;
		level.rooms.push_back(room);
	}
}

int main()
{
	YAML::Test::RunAll();

	//std::ifstream fin("test.yaml");

	//try {
	//	YAML::Parser parser(fin);
	//	if(!parser)
	//		return 0;

	//	YAML::Node doc;
	//	parser.GetNextDocument(doc);
	//	std::cout << doc;
	//} catch(YAML::Exception&) {
	//	std::cout << "Error parsing the yaml!\n";
	//}

	getchar();

	return 0;
}
