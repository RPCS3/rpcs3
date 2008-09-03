#include "crt.h"
#include "map.h"
#include "node.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include <iostream>

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

	void Map::Parse(Scanner *pScanner, const ParserState& state)
	{
		Clear();

		// split based on start token
		switch(pScanner->peek().type) {
			case TT_BLOCK_MAP_START: ParseBlock(pScanner, state); break;
			case TT_FLOW_MAP_START: ParseFlow(pScanner, state); break;
		}
	}

	void Map::ParseBlock(Scanner *pScanner, const ParserState& state)
	{
		// eat start token
		pScanner->pop();

		while(1) {
			if(pScanner->empty())
				throw ParserException(-1, -1, ErrorMsg::END_OF_MAP);

			Token token = pScanner->peek();
			if(token.type != TT_KEY && token.type != TT_BLOCK_END)
				throw ParserException(token.line, token.column, ErrorMsg::END_OF_MAP);

			pScanner->pop();
			if(token.type == TT_BLOCK_END)
				break;

			Node *pKey = new Node;
			Node *pValue = new Node;

			try {
				// grab key
				pKey->Parse(pScanner, state);

				// now grab value (optional)
				if(!pScanner->empty() && pScanner->peek().type == TT_VALUE) {
					pScanner->pop();
					pValue->Parse(pScanner, state);
				}

				m_data[pKey] = pValue;
			} catch(Exception& e) {
				delete pKey;
				delete pValue;
				throw e;
			}
		}
	}

	void Map::ParseFlow(Scanner *pScanner, const ParserState& state)
	{
		// eat start token
		pScanner->pop();

		while(1) {
			if(pScanner->empty())
				throw ParserException(-1, -1, ErrorMsg::END_OF_MAP_FLOW);

			Token& token = pScanner->peek();
			// first check for end
			if(token.type == TT_FLOW_MAP_END) {
				pScanner->pop();
				break;
			}

			// now it better be a key
			if(token.type != TT_KEY)
				throw ParserException(token.line, token.column, ErrorMsg::END_OF_MAP_FLOW);

			pScanner->pop();

			Node *pKey = new Node;
			Node *pValue = new Node;

			try {
				// grab key
				pKey->Parse(pScanner, state);

				// now grab value (optional)
				if(!pScanner->empty() && pScanner->peek().type == TT_VALUE) {
					pScanner->pop();
					pValue->Parse(pScanner, state);
				}

				// now eat the separator (or could be a map end, which we ignore - but if it's neither, then it's a bad node)
				Token& nextToken = pScanner->peek();
				if(nextToken.type == TT_FLOW_ENTRY)
					pScanner->pop();
				else if(nextToken.type != TT_FLOW_MAP_END)
					throw ParserException(nextToken.line, nextToken.column, ErrorMsg::END_OF_MAP_FLOW);

				m_data[pKey] = pValue;
			} catch(Exception& e) {
				// clean up and rethrow
				delete pKey;
				delete pValue;
				throw e;
			}
		}
	}

	void Map::Write(std::ostream& out, int indent, bool startedLine, bool onlyOneCharOnLine)
	{
		if(startedLine && !onlyOneCharOnLine)
			out << "\n";

		for(node_map::const_iterator it=m_data.begin();it!=m_data.end();++it) {
			if((startedLine && !onlyOneCharOnLine) || it != m_data.begin()) {
				for(int i=0;i<indent;i++)
					out << "  ";
			}

			out << "? ";
			it->first->Write(out, indent + 1, true, it!= m_data.begin() || !startedLine || onlyOneCharOnLine);

			for(int i=0;i<indent;i++)
				out << "  ";
			out << ": ";
			it->second->Write(out, indent + 1, true, true);
		}

		if(m_data.empty())
			out << "\n";
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
