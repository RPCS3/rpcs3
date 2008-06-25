#include "map.h"
#include "node.h"

namespace YAML
{
	Map::Map()
	{
	}

	Map::~Map()
	{
		for(node_map::const_iterator it=m_data.begin();it!=m_data.end();++it) {
			delete it->first;
			delete it->second;
		}
	}
}
