#include "parser.h"
#include "node.h"
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
	YAML::Node::Iterator it = node.begin();
	*it >> v.x;
	++it;
	*it >> v.y;
	++it;
	*it >> v.z;
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
	for(YAML::Node::Iterator it=rooms.begin();it!=rooms.end();++it) {
		Room room;
		*it >> room;
		level.rooms.push_back(room);
	}
}

int main()
{
	std::ifstream fin("test.yaml");

	try {
		YAML::Parser parser(fin);
		if(!parser)
			return 0;

		YAML::Document doc;
		parser.GetNextDocument(doc);

		const YAML::Node& root = doc.GetRoot();
		Level level;
		root >> level;
		std::cout << level;
	} catch(YAML::Exception&) {
		std::cout << "Error parsing the yaml!\n";
	}

	getchar();

	return 0;
}