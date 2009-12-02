#include "map.h"
#include "node.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include "emitter.h"

namespace YAML
{
	Map::Map()
	{
	}
	
	Map::Map(const node_map& data)
	{
		for(node_map::const_iterator it=data.begin();it!=data.end();++it) {
			std::auto_ptr<Node> pKey = it->first->Clone();
			std::auto_ptr<Node> pValue = it->second->Clone();
			AddEntry(pKey, pValue);
		}
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

	Content *Map::Clone() const
	{
		return new Map(m_data);
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

	void Map::Parse(Scanner *pScanner, ParserState& state)
	{
		Clear();

		// split based on start token
		switch(pScanner->peek().type) {
			case Token::BLOCK_MAP_START: ParseBlock(pScanner, state); break;
			case Token::FLOW_MAP_START: ParseFlow(pScanner, state); break;
			case Token::KEY: ParseCompact(pScanner, state); break;
			case Token::VALUE: ParseCompactWithNoKey(pScanner, state); break;
			default: break;
		}
	}

	void Map::ParseBlock(Scanner *pScanner, ParserState& state)
	{
		// eat start token
		pScanner->pop();
		state.PushCollectionType(ParserState::BLOCK_MAP);

		while(1) {
			if(pScanner->empty())
				throw ParserException(Mark::null(), ErrorMsg::END_OF_MAP);

			Token token = pScanner->peek();
			if(token.type != Token::KEY && token.type != Token::VALUE && token.type != Token::BLOCK_MAP_END)
				throw ParserException(token.mark, ErrorMsg::END_OF_MAP);

			if(token.type == Token::BLOCK_MAP_END) {
				pScanner->pop();
				break;
			}

			std::auto_ptr <Node> pKey(new Node), pValue(new Node);
			
			// grab key (if non-null)
			if(token.type == Token::KEY) {
				pScanner->pop();
				pKey->Parse(pScanner, state);
			}

			// now grab value (optional)
			if(!pScanner->empty() && pScanner->peek().type == Token::VALUE) {
				pScanner->pop();
				pValue->Parse(pScanner, state);
			}

			AddEntry(pKey, pValue);
		}

		state.PopCollectionType(ParserState::BLOCK_MAP);
	}

	void Map::ParseFlow(Scanner *pScanner, ParserState& state)
	{
		// eat start token
		pScanner->pop();
		state.PushCollectionType(ParserState::FLOW_MAP);

		while(1) {
			if(pScanner->empty())
				throw ParserException(Mark::null(), ErrorMsg::END_OF_MAP_FLOW);

			Token& token = pScanner->peek();
			// first check for end
			if(token.type == Token::FLOW_MAP_END) {
				pScanner->pop();
				break;
			}
			
			std::auto_ptr <Node> pKey(new Node), pValue(new Node);

			// grab key (if non-null)
			if(token.type == Token::KEY) {
				pScanner->pop();
				pKey->Parse(pScanner, state);
			}
			
			// now grab value (optional)
			if(!pScanner->empty() && pScanner->peek().type == Token::VALUE) {
				pScanner->pop();
				pValue->Parse(pScanner, state);
			}
			
			// now eat the separator (or could be a map end, which we ignore - but if it's neither, then it's a bad node)
			Token& nextToken = pScanner->peek();
			if(nextToken.type == Token::FLOW_ENTRY)
				pScanner->pop();
			else if(nextToken.type != Token::FLOW_MAP_END)
				throw ParserException(nextToken.mark, ErrorMsg::END_OF_MAP_FLOW);

			AddEntry(pKey, pValue);
		}

		state.PopCollectionType(ParserState::FLOW_MAP);
	}

	// ParseCompact
	// . Single "key: value" pair in a flow sequence
	void Map::ParseCompact(Scanner *pScanner, ParserState& state)
	{
		state.PushCollectionType(ParserState::COMPACT_MAP);
		std::auto_ptr <Node> pKey(new Node), pValue(new Node);

		// grab key
		pScanner->pop();
		pKey->Parse(pScanner, state);
			
		// now grab value (optional)
		if(!pScanner->empty() && pScanner->peek().type == Token::VALUE) {
			pScanner->pop();
			pValue->Parse(pScanner, state);
		}
			
		AddEntry(pKey, pValue);
		state.PopCollectionType(ParserState::COMPACT_MAP);
	}
	
	// ParseCompactWithNoKey
	// . Single ": value" pair in a flow sequence
	void Map::ParseCompactWithNoKey(Scanner *pScanner, ParserState& state)
	{
		state.PushCollectionType(ParserState::COMPACT_MAP);
		std::auto_ptr <Node> pKey(new Node), pValue(new Node);

		// grab value
		pScanner->pop();
		pValue->Parse(pScanner, state);

		AddEntry(pKey, pValue);
		state.PopCollectionType(ParserState::COMPACT_MAP);
	}
	
	void Map::AddEntry(std::auto_ptr<Node> pKey, std::auto_ptr<Node> pValue)
	{
		node_map::const_iterator it = m_data.find(pKey.get());
		if(it != m_data.end())
			return;
		
		m_data[pKey.release()] = pValue.release();
	}

	void Map::Write(Emitter& out) const
	{
		out << BeginMap;
		for(node_map::const_iterator it=m_data.begin();it!=m_data.end();++it)
			out << Key << *it->first << Value << *it->second;
		out << EndMap;
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
