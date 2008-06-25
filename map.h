#pragma once

#include "content.h"
#include <map>

namespace YAML
{
	class Node;

	class Map: public Content
	{
	public:
		Map();
		virtual ~Map();

	protected:
		typedef std::map <Node *, Node *> node_map;
		node_map m_data;
	};
}
