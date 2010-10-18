#include "map.h"
#include "yaml-cpp/node.h"
#include "yaml-cpp/eventhandler.h"
#include "yaml-cpp/exceptions.h"

namespace YAML
{
	Map::Map()
	{
	}

	Map::~Map()
	{
		Clear();
	}

	void Map::Clear()
	{
		for(node_map::const_iterator it=m_data.begin();it!=m_data.end();++it) {
			delete it->first;
			delete it->second;
		}
		m_data.clear();
	}

	bool Map::GetBegin(std::map <Node *, Node *, ltnode>::const_iterator& it) const
	{
		it = m_data.begin();
		return true;
	}

	bool Map::GetEnd(std::map <Node *, Node *, ltnode>::const_iterator& it) const
	{
		it = m_data.end();
		return true;
	}

	std::size_t Map::GetSize() const
	{
		return m_data.size();
	}

	void Map::Insert(std::auto_ptr<Node> pKey, std::auto_ptr<Node> pValue)
	{
		node_map::const_iterator it = m_data.find(pKey.get());
		if(it != m_data.end())
			return;
		
		m_data[pKey.release()] = pValue.release();
	}

	void Map::EmitEvents(AliasManager& am, EventHandler& eventHandler, const Mark& mark, const std::string& tag, anchor_t anchor) const
	{
		eventHandler.OnMapStart(mark, tag, anchor);
		for(node_map::const_iterator it=m_data.begin();it!=m_data.end();++it) {
			it->first->EmitEvents(am, eventHandler);
			it->second->EmitEvents(am, eventHandler);
		}
		eventHandler.OnMapEnd();
	}

	int Map::Compare(Content *pContent)
	{
		return -pContent->Compare(this);
	}

	int Map::Compare(Map *pMap)
	{
		node_map::const_iterator it = m_data.begin(), jt = pMap->m_data.begin();
		while(1) {
			if(it == m_data.end()) {
				if(jt == pMap->m_data.end())
					return 0;
				else
					return -1;
			}
			if(jt == pMap->m_data.end())
				return 1;

			int cmp = it->first->Compare(*jt->first);
			if(cmp != 0)
				return cmp;

			cmp = it->second->Compare(*jt->second);
			if(cmp != 0)
				return cmp;
		}

		return 0;
	}
}
